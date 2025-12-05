/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_hold_single_double

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/keys.h>
#include <dt-bindings/zmk/keys.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

/* forward declare listener symbol so ZMK_EVENT_RAISE_AT can reference it from helpers */
extern const struct zmk_listener zmk_listener_behavior_hsd;

#define ZMK_BHV_HSD_MAX_HELD CONFIG_ZMK_BEHAVIOR_HOLD_SINGLE_DOUBLE_MAX_HELD
#define ZMK_BHV_HSD_POSITION_FREE UINT32_MAX

struct behavior_hsd_config {
    uint32_t tapping_term_ms;
    size_t behavior_count; /* usually 3: hold, single, double */
    struct zmk_behavior_binding *behaviors;
};

struct active_hsd {
    int counter; /* number of taps detected */
    uint32_t position;
    bool is_pressed;
    const struct behavior_hsd_config *config;

    bool timer_cancelled;
    bool decided;
    int64_t release_at;
    struct k_work_delayable timer_work;
    enum {
        HSD_STATUS_UNDECIDED = 0,
        HSD_STATUS_TAP,
        HSD_STATUS_HOLD,
    } status;
    int decided_idx;
};

static struct active_hsd active_hsds[ZMK_BHV_HSD_MAX_HELD] = {};

/* Capture support (copied pattern from behavior_hold_tap.c) */
#define ZMK_BHV_HSD_MAX_CAPTURED_EVENTS 40
static const zmk_event_t *captured_events[ZMK_BHV_HSD_MAX_CAPTURED_EVENTS] = {};
static struct active_hsd *undecided_hsd = NULL;

static int capture_event(const zmk_event_t *event) {
    for (int i = 0; i < ZMK_BHV_HSD_MAX_CAPTURED_EVENTS; i++) {
        if (captured_events[i] == NULL) {
            captured_events[i] = event;
            return 0;
        }
    }
    return -ENOMEM;
}

static struct zmk_position_state_changed *find_captured_keydown_event(uint32_t position) {
    struct zmk_position_state_changed *last_match = NULL;
    for (int i = 0; i < ZMK_BHV_HSD_MAX_CAPTURED_EVENTS; i++) {
        const zmk_event_t *eh = captured_events[i];
        if (eh == NULL) {
            return last_match;
        }
        struct zmk_position_state_changed *position_event = as_zmk_position_state_changed(eh);
        if (position_event == NULL) {
            continue;
        }

        if (position_event->position == position && position_event->state) {
            last_match = position_event;
        }
    }
    return last_match;
}

static void release_captured_events() {
    if (undecided_hsd != NULL) {
        return;
    }

    for (int i = 0; i < ZMK_BHV_HSD_MAX_CAPTURED_EVENTS; i++) {
        const zmk_event_t *captured_event = captured_events[i];
        if (captured_event == NULL) {
            return;
        }
        captured_events[i] = NULL;
        if (undecided_hsd != NULL) {
            k_msleep(10);
        }

        struct zmk_position_state_changed *position_event;
        struct zmk_keycode_state_changed *modifier_event;
        if ((position_event = as_zmk_position_state_changed(captured_event)) != NULL) {
            LOG_DBG("Releasing key position event for position %d %s", position_event->position,
                    (position_event->state ? "pressed" : "released"));
        } else if ((modifier_event = as_zmk_keycode_state_changed(captured_event)) != NULL) {
            LOG_DBG("Releasing mods changed event 0x%02X %s", modifier_event->keycode,
                    (modifier_event->state ? "pressed" : "released"));
        }
        ZMK_EVENT_RAISE_AT(captured_event, behavior_hsd);
    }
}

static struct active_hsd *find_hsd(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_HSD_MAX_HELD; i++) {
        if (active_hsds[i].position == position && !active_hsds[i].timer_cancelled) {
            return &active_hsds[i];
        }
    }
    return NULL;
}

static int new_hsd(uint32_t position, const struct behavior_hsd_config *config,
                   struct active_hsd **hsd) {
    for (int i = 0; i < ZMK_BHV_HSD_MAX_HELD; i++) {
        struct active_hsd *ref = &active_hsds[i];
        if (ref->position == ZMK_BHV_HSD_POSITION_FREE) {
            ref->counter = 0;
            ref->position = position;
            ref->config = config;
            ref->is_pressed = true;
            ref->timer_cancelled = false;
            ref->decided = false;
            ref->release_at = 0;
            *hsd = ref;
            return 0;
        }
    }
    return -ENOMEM;
}

static void clear_hsd(struct active_hsd *hsd) { hsd->position = ZMK_BHV_HSD_POSITION_FREE; }

static int stop_timer(struct active_hsd *hsd) {
    int ret = k_work_cancel_delayable(&hsd->timer_work);
    if (ret == -EINPROGRESS) {
        hsd->timer_cancelled = true;
    }
    return ret;
}

static void reset_timer(struct active_hsd *hsd, int64_t now_ts) {
    hsd->release_at = now_ts + hsd->config->tapping_term_ms;
    int32_t ms_left = hsd->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_work_schedule(&hsd->timer_work, K_MSEC(ms_left));
        LOG_DBG("hsd: reset timer at position %d", hsd->position);
    }
}

static inline int press_behavior(struct active_hsd *hsd, int idx, int64_t timestamp) {
    if (idx < 0 || (size_t)idx >= hsd->config->behavior_count) {
        return 0;
    }
    hsd->decided = true;
    hsd->decided_idx = idx;
    hsd->status = (idx == 0) ? HSD_STATUS_HOLD : HSD_STATUS_TAP;
    struct zmk_behavior_binding binding = hsd->config->behaviors[idx];
    struct zmk_behavior_binding_event event = {.position = hsd->position, .timestamp = timestamp};
    return behavior_keymap_binding_pressed(&binding, event);
}

static inline int release_behavior(struct active_hsd *hsd, int idx, int64_t timestamp) {
    if (idx < 0 || (size_t)idx >= hsd->config->behavior_count) {
        return 0;
    }
    struct zmk_behavior_binding binding = hsd->config->behaviors[idx];
    struct zmk_behavior_binding_event event = {.position = hsd->position, .timestamp = timestamp};
    clear_hsd(hsd);
    return behavior_keymap_binding_released(&binding, event);
}

static int on_hsd_pressed(struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_hsd_config *cfg = dev->config;
    struct active_hsd *hsd = find_hsd(event.position);
    if (hsd == NULL) {
        if (new_hsd(event.position, cfg, &hsd) == -ENOMEM) {
            LOG_ERR("Unable to create new hsd. No space.");
            return ZMK_BEHAVIOR_OPAQUE;
        }
        LOG_DBG("%d created new hsd", event.position);
        /* mark as the currently undecided hsd so we can capture other events */
        undecided_hsd = hsd;
    }
    hsd->is_pressed = true;
    stop_timer(hsd);

    if (hsd->counter < (int)cfg->behavior_count) {
        hsd->counter++;
    }

    /* If counter equals configured maximum number of bindings, decide now by triggering the last
     * binding */
    if (hsd->counter == (int)cfg->behavior_count) {
        /* Decide immediately when counter reaches maximum: press last binding and release captured
         * events */
        press_behavior(hsd, hsd->counter - 1, event.timestamp);
        undecided_hsd = NULL;
        release_captured_events();
        return ZMK_EV_EVENT_BUBBLE;
    }

    reset_timer(hsd, event.timestamp);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_hsd_released(struct zmk_behavior_binding *binding,
                           struct zmk_behavior_binding_event event) {
    LOG_DBG("%d hsd released", event.position);
    struct active_hsd *hsd = find_hsd(event.position);
    if (hsd == NULL) {
        LOG_ERR("ACTIVE HSD CLEARED TOO EARLY");
        return ZMK_BEHAVIOR_OPAQUE;
    }
    hsd->is_pressed = false;
    if (hsd->decided) {
        /* If we've already decided and pressed a behavior, release it now */
        release_behavior(hsd, hsd->decided_idx, event.timestamp);
    } else {
        /* Not decided yet: decide now as key-up (single/double) */
        /* choose index based on counter */
        int idx = (hsd->counter == 1) ? 1
                                      : ((hsd->config->behavior_count >= 3 && hsd->counter >= 2)
                                             ? 2
                                             : (hsd->config->behavior_count - 1));
        press_behavior(hsd, idx, event.timestamp);
        release_behavior(hsd, idx, event.timestamp);
        undecided_hsd = NULL;
        release_captured_events();
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

void behavior_hsd_timer_handler(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);
    struct active_hsd *hsd = CONTAINER_OF(d_work, struct active_hsd, timer_work);
    if (hsd->position == ZMK_BHV_HSD_POSITION_FREE) {
        return;
    }
    if (hsd->timer_cancelled) {
        return;
    }

    LOG_DBG("hsd timer fired for pos %d, counter %d", hsd->position, hsd->counter);

    /* Decide based on counter and is_pressed */
    if (hsd->counter == 0) {
        clear_hsd(hsd);
        return;
    }

    if (hsd->is_pressed) {
        /* Held beyond tapping term -> treat as HOLD (binding index 0) */
        press_behavior(hsd, 0, hsd->release_at);
        undecided_hsd = NULL;
        release_captured_events();
        return;
    }

    /* Not pressed when timer expired -> decide single or double tap depending on counter */
    if (hsd->counter == 1) {
        /* single tap -> binding index 1 */
        press_behavior(hsd, 1, hsd->release_at);
        release_behavior(hsd, 1, hsd->release_at);
        undecided_hsd = NULL;
        release_captured_events();
    } else if (hsd->counter >= 2) {
        /* double tap -> binding index 2 if present, otherwise use last */
        int idx = (hsd->config->behavior_count >= 3) ? 2 : (hsd->config->behavior_count - 1);
        press_behavior(hsd, idx, hsd->release_at);
        release_behavior(hsd, idx, hsd->release_at);
        undecided_hsd = NULL;
        release_captured_events();
    }
}

static const struct behavior_driver_api behavior_hsd_driver_api = {
    .binding_pressed = on_hsd_pressed,
    .binding_released = on_hsd_released,
};

static int behavior_hsd_listener(const zmk_event_t *eh) {
    struct zmk_position_state_changed *evp = as_zmk_position_state_changed(eh);
    struct zmk_keycode_state_changed *evk = as_zmk_keycode_state_changed(eh);

    if (undecided_hsd == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (evp != NULL) {
        /* If the event is from the undecided key itself, let it bubble (behavior handlers handle
         * it) */
        if (evp->position == undecided_hsd->position) {
            if (evp->state) {
                LOG_ERR("hsd listener should be called before most other listeners!");
                return ZMK_EV_EVENT_BUBBLE;
            } else {
                return ZMK_EV_EVENT_BUBBLE;
            }
        }

        if (!evp->state && find_captured_keydown_event(evp->position) == NULL) {
            /* no keydown captured for this position, bubble */
            return ZMK_EV_EVENT_BUBBLE;
        }

        capture_event(eh);
        return ZMK_EV_EVENT_CAPTURED;
    }

    if (evk != NULL) {
        /* capture modifier key events while undecided */
        if (!is_mod(evk->usage_page, evk->keycode)) {
            return ZMK_EV_EVENT_BUBBLE;
        }
        capture_event(eh);
        return ZMK_EV_EVENT_CAPTURED;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(behavior_hsd, behavior_hsd_listener);
ZMK_SUBSCRIPTION(behavior_hsd, zmk_position_state_changed);
ZMK_SUBSCRIPTION(behavior_hsd, zmk_keycode_state_changed);

static int behavior_hsd_init(const struct device *dev) {
    static bool first = true;
    if (first) {
        for (int i = 0; i < ZMK_BHV_HSD_MAX_HELD; i++) {
            k_work_init_delayable(&active_hsds[i].timer_work, behavior_hsd_timer_handler);
            clear_hsd(&active_hsds[i]);
        }
    }
    first = false;
    return 0;
}

#define _TRANSFORM_ENTRY(idx, node) ZMK_KEYMAP_EXTRACT_BINDING(idx, node)

#define TRANSFORMED_BINDINGS(node)                                                                 \
    {LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_ENTRY, (, ), DT_DRV_INST(node))}

#define KP_INST(n)                                                                                 \
    static struct zmk_behavior_binding                                                             \
        behavior_hsd_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =                        \
            TRANSFORMED_BINDINGS(n);                                                               \
    static struct behavior_hsd_config behavior_hsd_config_##n = {                                  \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                       \
        .behaviors = behavior_hsd_config_##n##_bindings,                                           \
        .behavior_count = DT_INST_PROP_LEN(n, bindings)};                                          \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_hsd_init, NULL, NULL, &behavior_hsd_config_##n,            \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_hsd_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
