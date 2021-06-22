/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_tap_dance

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>

#include <zmk/matrix.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ZMK_BHV_TAP_DANCE_MAX_HELD 10

#define ZMK_BHV_TAP_DANCE_POSITION_FREE ULONG_MAX

struct behavior_tap_dance_config {
    uint32_t tapping_term_ms;
    int behavior_count;
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
    struct k_delayed_work release_timer;
};

struct active_tap_dance active_tap_dances[ZMK_BHV_TAP_DANCE_MAX_HELD] = {};

static struct active_tap_dance *store_tap_dance(uint32_t position,
                                                const struct behavior_tap_dance_config *config) {
    // Create a new tap dance instance
    for (int i = 0; i < ZMK_BHV_TAP_DANCE_MAX_HELD; i++) {
        struct active_tap_dance *const tap_dance = &active_tap_dances[i];
        if (tap_dance->position != ZMK_BHV_TAP_DANCE_POSITION_FREE || tap_dance->timer_cancelled) {
            continue;
        }
        tap_dance->counter = 1;
        tap_dance->position = position;
        tap_dance->config = config;
        tap_dance->release_at = 0;
        tap_dance->is_pressed = true;
        tap_dance->timer_started = true;
        tap_dance->timer_cancelled = false;
        tap_dance->tap_dance_decided = false;
        return tap_dance;
    }
    return NULL;
}

static struct active_tap_dance *find_tap_dance(uint32_t position) {
    // Search for existing tap dances at position
    for (int i = 0; i < ZMK_BHV_TAP_DANCE_MAX_HELD; i++) {
        if (active_tap_dances[i].position == position && !active_tap_dances[i].timer_cancelled) {
            return &active_tap_dances[i];
        }
    }
    return NULL;
}

static int stop_timer(struct active_tap_dance *tap_dance) {
    int timer_cancel_result = k_delayed_work_cancel(&tap_dance->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        tap_dance->timer_cancelled = true;
    }
    return timer_cancel_result;
}

static void clear_tap_dance(struct active_tap_dance *tap_dance) {
    // FREE position, reset counter.
    tap_dance->position = ZMK_BHV_TAP_DANCE_POSITION_FREE;
    tap_dance->counter = 1;
}

static void reset_timer(struct active_tap_dance *tap_dance,
                        struct zmk_behavior_binding_event event) {
    // Start the timer if one hasn't started already. Reset it if one exists.
    tap_dance->release_at = event.timestamp + tap_dance->config->tapping_term_ms;
    int32_t ms_left = tap_dance->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_delayed_work_submit(&tap_dance->release_timer, K_MSEC(ms_left));
    }
}

static inline int press_tap_dance_behavior(struct active_tap_dance *tap_dance, int64_t timestamp) {
    // Press tap dance binding
    struct zmk_behavior_binding binding =
        (tap_dance->counter < tap_dance->config->behavior_count)
            ? tap_dance->config->behaviors[(tap_dance->counter) - 1]
            : tap_dance->config->behaviors[(tap_dance->config->behavior_count) - 1];
    struct zmk_behavior_binding_event event = {
        .position = tap_dance->position,
        .timestamp = timestamp,
    };
    behavior_keymap_binding_pressed(&binding, event);
    return 0;
}

static inline int release_tap_dance_behavior(struct active_tap_dance *tap_dance,
                                             int64_t timestamp) {
    // Release tap dance binding
    struct zmk_behavior_binding binding =
        (tap_dance->counter < tap_dance->config->behavior_count)
            ? tap_dance->config->behaviors[(tap_dance->counter) - 1]
            : tap_dance->config->behaviors[(tap_dance->config->behavior_count) - 1];
    struct zmk_behavior_binding_event event = {
        .position = tap_dance->position,
        .timestamp = timestamp,
    };
    behavior_keymap_binding_released(&binding, event);
    clear_tap_dance(tap_dance);
    return 0;
}

static int on_tap_dance_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    // Code executes when the tap dance key is pressed.
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_tap_dance_config *cfg = dev->config;
    struct active_tap_dance *tap_dance;
    tap_dance = find_tap_dance(event.position);
    // No active tap dance found on keypress location, create a new one
    if (tap_dance == NULL) {
        tap_dance = store_tap_dance(event.position, cfg);
        LOG_DBG("%d creating new tap dance", event.position);
        reset_timer(tap_dance, event);
        return ZMK_BEHAVIOR_OPAQUE;
    }
    // An active tap dance was found at the keypress location. Stop the timer.
    tap_dance->is_pressed = true;
    LOG_DBG("%d tap dance re-pressed", event.position);
    stop_timer(tap_dance);
    // Increment the counter. Check if the counter has reached the maximum behavior number.
    tap_dance->counter++;
    if (tap_dance->counter >= cfg->behavior_count) {
        // Counter is greater than or equal to the maximum number of behaviors.
        // Resolve tap dance immediately.
        press_tap_dance_behavior(tap_dance, event.timestamp);
        tap_dance->tap_dance_decided = true;
        return ZMK_BEHAVIOR_OPAQUE;
    }
    // Counter is less than maximum number of behaviors, reset timer.
    reset_timer(tap_dance, event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_tap_dance_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    // Code executes when the tap dance key is released.
    LOG_DBG("%d tap dance keybind released", event.position);
    struct active_tap_dance *tap_dance = find_tap_dance(event.position);
    if (tap_dance == NULL) {
        LOG_ERR("ACTIVE TAP DANCE CLEARED TOO EARLY");
        return ZMK_BEHAVIOR_OPAQUE;
    }
    tap_dance->is_pressed = false;
    // An active tap dance has already been decided and is registering keypresses.
    // Release tap dance on key-release.
    if (tap_dance->tap_dance_decided) {
        LOG_DBG("Maximum count reached and/or key is held! Counter reached: %d",
                tap_dance->counter);
        release_tap_dance_behavior(tap_dance, event.timestamp);
        tap_dance->tap_dance_decided = false;
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

void behavior_tap_dance_timer_handler(struct k_work *item) {
    LOG_DBG("Timer Handler executes");
    // Timer for tap dance has ran out.
    struct active_tap_dance *tap_dance = CONTAINER_OF(item, struct active_tap_dance, release_timer);
    if (tap_dance->position == ZMK_BHV_TAP_DANCE_POSITION_FREE) {
        return;
    }
    tap_dance->timer_cancelled = false;
    press_tap_dance_behavior(tap_dance, tap_dance->release_at);
    // Tap dance is held, store that information to prepare for key release
    if (tap_dance->is_pressed) {
        tap_dance->tap_dance_decided = true;
        return;
    }
    // Tap dance is not held. Release the tap dance.
    LOG_DBG("Tap dance releases immediately! Counter reached: %d", tap_dance->counter);
    release_tap_dance_behavior(tap_dance, tap_dance->release_at);
    return;
}

static const struct behavior_driver_api behavior_tap_dance_driver_api = {
    .binding_pressed = on_tap_dance_binding_pressed,
    .binding_released = on_tap_dance_binding_released,
};

static int tap_dance_position_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_tap_dance, tap_dance_position_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_tap_dance, zmk_position_state_changed);

static int tap_dance_position_state_changed_listener(const zmk_event_t *eh) {
    // A keypress with a different position than the last tap dance has been recorded.
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return 0;
    }
    for (int i = 0; i < ZMK_BHV_TAP_DANCE_MAX_HELD; i++) {
        struct active_tap_dance *tap_dance = &active_tap_dances[i];
        if (tap_dance->position == ZMK_BHV_TAP_DANCE_POSITION_FREE) {
            continue;
        }
        if (tap_dance->position != ev->position && tap_dance->timer_started) {
            // Resolve tap dance immediately before new keypress.
            LOG_DBG("Tap dance interrupted, position changed.");
            stop_timer(tap_dance);
            press_tap_dance_behavior(tap_dance, tap_dance->release_at);
            release_tap_dance_behavior(tap_dance, tap_dance->release_at);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    },

#define TRANSFORMED_BINDINGS(node)                                                                 \
    { UTIL_LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_ENTRY, node) }

static int behavior_tap_dance_init(const struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_TAP_DANCE_MAX_HELD; i++) {
            k_delayed_work_init(&active_tap_dances[i].release_timer,
                                behavior_tap_dance_timer_handler);
            active_tap_dances[i].position = ZMK_BHV_TAP_DANCE_POSITION_FREE;
        }
    }
    init_first_run = false;
    return 0;
}

struct behavior_tap_dance_data {};
static struct behavior_tap_dance_data behavior_tap_dance_data;

#define KP_INST(n)                                                                                 \
    static struct zmk_behavior_binding                                                             \
        behavior_tap_dance_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =                  \
            TRANSFORMED_BINDINGS(n);                                                               \
    static struct behavior_tap_dance_config behavior_tap_dance_config_##n = {                      \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                       \
        .behaviors = behavior_tap_dance_config_##n##_bindings,                                     \
        .behavior_count = DT_INST_PROP_LEN(n, bindings)};                                          \
    DEVICE_AND_API_INIT(behavior_tap_dance_##n, DT_INST_LABEL(n), behavior_tap_dance_init,         \
                        &behavior_tap_dance_data, &behavior_tap_dance_config_##n, APPLICATION,     \
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_tap_dance_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif