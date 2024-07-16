/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_tap_dance

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ZMK_BHV_TAP_DANCE_MAX_HELD 10

#define ZMK_BHV_TAP_DANCE_POSITION_FREE UINT32_MAX

struct behavior_tap_dance_config {
    uint32_t tapping_term_ms;
    size_t behavior_count;
    struct zmk_behavior_binding *behaviors;
};

struct active_tap_dance {
    // Tap Dance Data
    int counter;
    uint32_t position;
    uint32_t param1;
    uint32_t param2;
    bool is_pressed;
    const struct behavior_tap_dance_config *config;

    // Timer Data
    bool timer_started;
    bool timer_cancelled;
    bool tap_dance_decided;
    int64_t release_at;
    struct k_work_delayable release_timer;
};

struct active_tap_dance active_tap_dances[ZMK_BHV_TAP_DANCE_MAX_HELD] = {};

static struct active_tap_dance *find_tap_dance(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_TAP_DANCE_MAX_HELD; i++) {
        if (active_tap_dances[i].position == position && !active_tap_dances[i].timer_cancelled) {
            return &active_tap_dances[i];
        }
    }
    return NULL;
}

static int new_tap_dance(uint32_t position, const struct behavior_tap_dance_config *config,
                         struct active_tap_dance **tap_dance) {
    for (int i = 0; i < ZMK_BHV_TAP_DANCE_MAX_HELD; i++) {
        struct active_tap_dance *const ref_dance = &active_tap_dances[i];
        if (ref_dance->position == ZMK_BHV_TAP_DANCE_POSITION_FREE) {
            ref_dance->counter = 0;
            ref_dance->position = position;
            ref_dance->config = config;
            ref_dance->release_at = 0;
            ref_dance->is_pressed = true;
            ref_dance->timer_started = true;
            ref_dance->timer_cancelled = false;
            ref_dance->tap_dance_decided = false;
            *tap_dance = ref_dance;
            return 0;
        }
    }
    return -ENOMEM;
}

static void clear_tap_dance(struct active_tap_dance *tap_dance) {
    tap_dance->position = ZMK_BHV_TAP_DANCE_POSITION_FREE;
}

static int stop_timer(struct active_tap_dance *tap_dance) {
    int timer_cancel_result = k_work_cancel_delayable(&tap_dance->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        tap_dance->timer_cancelled = true;
    }
    return timer_cancel_result;
}

static void reset_timer(struct active_tap_dance *tap_dance,
                        struct zmk_behavior_binding_event event) {
    tap_dance->release_at = event.timestamp + tap_dance->config->tapping_term_ms;
    int32_t ms_left = tap_dance->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_work_schedule(&tap_dance->release_timer, K_MSEC(ms_left));
        LOG_DBG("Successfully reset timer at position %d", tap_dance->position);
    }
}

static inline int press_tap_dance_behavior(struct active_tap_dance *tap_dance, int64_t timestamp) {
    tap_dance->tap_dance_decided = true;
    struct zmk_behavior_binding binding = tap_dance->config->behaviors[tap_dance->counter - 1];
    struct zmk_behavior_binding_event event = {
        .position = tap_dance->position,
        .timestamp = timestamp,
    };
    return behavior_keymap_binding_pressed(&binding, event);
}

static inline int release_tap_dance_behavior(struct active_tap_dance *tap_dance,
                                             int64_t timestamp) {
    struct zmk_behavior_binding binding = tap_dance->config->behaviors[tap_dance->counter - 1];
    struct zmk_behavior_binding_event event = {
        .position = tap_dance->position,
        .timestamp = timestamp,
    };
    clear_tap_dance(tap_dance);
    return behavior_keymap_binding_released(&binding, event);
}

static int on_tap_dance_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_tap_dance_config *cfg = dev->config;
    struct active_tap_dance *tap_dance;
    tap_dance = find_tap_dance(event.position);
    if (tap_dance == NULL) {
        if (new_tap_dance(event.position, cfg, &tap_dance) == -ENOMEM) {
            LOG_ERR("Unable to create new tap dance. Insufficient space in active_tap_dances[].");
            return ZMK_BEHAVIOR_OPAQUE;
        }
        LOG_DBG("%d created new tap dance", event.position);
    }
    tap_dance->is_pressed = true;
    LOG_DBG("%d tap dance pressed", event.position);
    stop_timer(tap_dance);
    // Increment the counter on keypress. If the counter has reached its maximum
    // value, invoke the last binding available.
    if (tap_dance->counter < cfg->behavior_count) {
        tap_dance->counter++;
    }
    if (tap_dance->counter == cfg->behavior_count) {
        // LOG_DBG("Tap dance has been decided via maximum counter value");
        press_tap_dance_behavior(tap_dance, event.timestamp);
        return ZMK_EV_EVENT_BUBBLE;
    }
    reset_timer(tap_dance, event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_tap_dance_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    LOG_DBG("%d tap dance keybind released", event.position);
    struct active_tap_dance *tap_dance = find_tap_dance(event.position);
    if (tap_dance == NULL) {
        LOG_ERR("ACTIVE TAP DANCE CLEARED TOO EARLY");
        return ZMK_BEHAVIOR_OPAQUE;
    }
    tap_dance->is_pressed = false;
    if (tap_dance->tap_dance_decided) {
        release_tap_dance_behavior(tap_dance, event.timestamp);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

void behavior_tap_dance_timer_handler(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);
    struct active_tap_dance *tap_dance =
        CONTAINER_OF(d_work, struct active_tap_dance, release_timer);
    if (tap_dance->position == ZMK_BHV_TAP_DANCE_POSITION_FREE) {
        return;
    }
    if (tap_dance->timer_cancelled) {
        return;
    }
    LOG_DBG("Tap dance has been decided via timer. Counter reached: %d", tap_dance->counter);
    press_tap_dance_behavior(tap_dance, tap_dance->release_at);
    if (tap_dance->is_pressed) {
        return;
    }
    release_tap_dance_behavior(tap_dance, tap_dance->release_at);
}

static const struct behavior_driver_api behavior_tap_dance_driver_api = {
    .binding_pressed = on_tap_dance_binding_pressed,
    .binding_released = on_tap_dance_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

static int tap_dance_position_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_tap_dance, tap_dance_position_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_tap_dance, zmk_position_state_changed);

static int tap_dance_position_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    if (!ev->state) {
        LOG_DBG("Ignore upstroke at position %d.", ev->position);
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (int i = 0; i < ZMK_BHV_TAP_DANCE_MAX_HELD; i++) {
        struct active_tap_dance *tap_dance = &active_tap_dances[i];
        if (tap_dance->position == ZMK_BHV_TAP_DANCE_POSITION_FREE) {
            continue;
        }
        if (tap_dance->position == ev->position) {
            continue;
        }
        stop_timer(tap_dance);
        LOG_DBG("Tap dance interrupted, activating tap-dance at %d", tap_dance->position);
        if (!tap_dance->tap_dance_decided) {
            press_tap_dance_behavior(tap_dance, ev->timestamp);
            if (!tap_dance->is_pressed) {
                release_tap_dance_behavior(tap_dance, ev->timestamp);
            }
            return ZMK_EV_EVENT_BUBBLE;
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int behavior_tap_dance_init(const struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_TAP_DANCE_MAX_HELD; i++) {
            k_work_init_delayable(&active_tap_dances[i].release_timer,
                                  behavior_tap_dance_timer_handler);
            clear_tap_dance(&active_tap_dances[i]);
        }
    }
    init_first_run = false;
    return 0;
}

#define _TRANSFORM_ENTRY(idx, node) ZMK_KEYMAP_EXTRACT_BINDING(idx, node)

#define TRANSFORMED_BINDINGS(node)                                                                 \
    { LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_ENTRY, (, ), DT_DRV_INST(node)) }

#define KP_INST(n)                                                                                 \
    static struct zmk_behavior_binding                                                             \
        behavior_tap_dance_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =                  \
            TRANSFORMED_BINDINGS(n);                                                               \
    static struct behavior_tap_dance_config behavior_tap_dance_config_##n = {                      \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                       \
        .behaviors = behavior_tap_dance_config_##n##_bindings,                                     \
        .behavior_count = DT_INST_PROP_LEN(n, bindings)};                                          \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_tap_dance_init, NULL, NULL,                                \
                            &behavior_tap_dance_config_##n, POST_KERNEL,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_tap_dance_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif