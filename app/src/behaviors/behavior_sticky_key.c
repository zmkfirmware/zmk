/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sticky_key

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>

#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ZMK_BHV_STICKY_KEY_MAX_HELD 10

#define ZMK_BHV_STICKY_KEY_POSITION_FREE ULONG_MAX

struct behavior_sticky_key_config {
    uint32_t release_after_ms;
    bool quick_release;
    struct zmk_behavior_binding behavior;
};

struct active_sticky_key {
    uint32_t position;
    uint32_t param1;
    uint32_t param2;
    const struct behavior_sticky_key_config *config;
    // timer data.
    bool timer_started;
    bool timer_cancelled;
    int64_t release_at;
    struct k_delayed_work release_timer;
    // usage page and keycode for the key that is being modified by this sticky key
    uint8_t modified_key_usage_page;
    uint32_t modified_key_keycode;
};

struct active_sticky_key active_sticky_keys[ZMK_BHV_STICKY_KEY_MAX_HELD] = {};

static struct active_sticky_key *store_sticky_key(uint32_t position, uint32_t param1,
                                                  uint32_t param2,
                                                  const struct behavior_sticky_key_config *config) {
    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        struct active_sticky_key *const sticky_key = &active_sticky_keys[i];
        if (sticky_key->position != ZMK_BHV_STICKY_KEY_POSITION_FREE ||
            sticky_key->timer_cancelled) {
            continue;
        }
        sticky_key->position = position;
        sticky_key->param1 = param1;
        sticky_key->param2 = param2;
        sticky_key->config = config;
        sticky_key->release_at = 0;
        sticky_key->timer_cancelled = false;
        sticky_key->timer_started = false;
        sticky_key->modified_key_usage_page = 0;
        sticky_key->modified_key_keycode = 0;
        return sticky_key;
    }
    return NULL;
}

static void clear_sticky_key(struct active_sticky_key *sticky_key) {
    sticky_key->position = ZMK_BHV_STICKY_KEY_POSITION_FREE;
}

static struct active_sticky_key *find_sticky_key(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        if (active_sticky_keys[i].position == position && !active_sticky_keys[i].timer_cancelled) {
            return &active_sticky_keys[i];
        }
    }
    return NULL;
}

static inline int press_sticky_key_behavior(struct active_sticky_key *sticky_key,
                                            int64_t timestamp) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = sticky_key->config->behavior.behavior_dev,
        .param1 = sticky_key->param1,
        .param2 = sticky_key->param2,
    };
    struct zmk_behavior_binding_event event = {
        .position = sticky_key->position,
        .timestamp = timestamp,
    };
    return behavior_keymap_binding_pressed(&binding, event);
}

static inline int release_sticky_key_behavior(struct active_sticky_key *sticky_key,
                                              int64_t timestamp) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = sticky_key->config->behavior.behavior_dev,
        .param1 = sticky_key->param1,
        .param2 = sticky_key->param2,
    };
    struct zmk_behavior_binding_event event = {
        .position = sticky_key->position,
        .timestamp = timestamp,
    };

    clear_sticky_key(sticky_key);
    return behavior_keymap_binding_released(&binding, event);
}

static int stop_timer(struct active_sticky_key *sticky_key) {
    int timer_cancel_result = k_delayed_work_cancel(&sticky_key->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        sticky_key->timer_cancelled = true;
    }
    return timer_cancel_result;
}

static int on_sticky_key_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sticky_key_config *cfg = dev->config;
    struct active_sticky_key *sticky_key;
    sticky_key = find_sticky_key(event.position);
    if (sticky_key != NULL) {
        stop_timer(sticky_key);
        release_sticky_key_behavior(sticky_key, event.timestamp);
    }
    sticky_key = store_sticky_key(event.position, binding->param1, binding->param2, cfg);
    if (sticky_key == NULL) {
        LOG_ERR("unable to store sticky key, did you press more than %d sticky_key?",
                ZMK_BHV_STICKY_KEY_MAX_HELD);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    press_sticky_key_behavior(sticky_key, event.timestamp);
    LOG_DBG("%d new sticky_key", event.position);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_sticky_key_binding_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    struct active_sticky_key *sticky_key = find_sticky_key(event.position);
    if (sticky_key == NULL) {
        LOG_ERR("ACTIVE STICKY KEY CLEARED TOO EARLY");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    if (sticky_key->modified_key_usage_page != 0 && sticky_key->modified_key_keycode != 0) {
        LOG_DBG("Another key was pressed while the sticky key was pressed. Act like a normal key.");
        return release_sticky_key_behavior(sticky_key, event.timestamp);
    }

    // No other key was pressed. Start the timer.
    sticky_key->timer_started = true;
    sticky_key->release_at = event.timestamp + sticky_key->config->release_after_ms;
    // adjust timer in case this behavior was queued by a hold-tap
    int32_t ms_left = sticky_key->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_delayed_work_submit(&sticky_key->release_timer, K_MSEC(ms_left));
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_sticky_key_driver_api = {
    .binding_pressed = on_sticky_key_binding_pressed,
    .binding_released = on_sticky_key_binding_released,
};

static int sticky_key_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_sticky_key, sticky_key_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_sticky_key, zmk_keycode_state_changed);

static int sticky_key_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        struct active_sticky_key *sticky_key = &active_sticky_keys[i];
        if (sticky_key->position == ZMK_BHV_STICKY_KEY_POSITION_FREE) {
            continue;
        }

        if (strcmp(sticky_key->config->behavior.behavior_dev, "KEY_PRESS") == 0 &&
            HID_USAGE_ID(sticky_key->param1) == ev->keycode &&
            (HID_USAGE_PAGE(sticky_key->param1) & 0xFF) == ev->usage_page &&
            SELECT_MODS(sticky_key->param1) == ev->implicit_modifiers) {
            // don't catch key down events generated by the sticky key behavior itself
            continue;
        }

        // If events were queued, the timer event may be queued late or not at all.
        // Release the sticky key if the timer should've run out in the meantime.
        if (sticky_key->release_at != 0 && ev->timestamp > sticky_key->release_at) {
            stop_timer(sticky_key);
            release_sticky_key_behavior(sticky_key, sticky_key->release_at);
            continue;
        }

        if (ev->state) { // key down
            if (sticky_key->modified_key_usage_page != 0 || sticky_key->modified_key_keycode != 0) {
                // this sticky key is already in use for a keycode
                continue;
            }
            if (sticky_key->timer_started) {
                stop_timer(sticky_key);
                if (sticky_key->config->quick_release) {
                    // continue processing the event. Release the sticky key afterwards.
                    ZMK_EVENT_RAISE_AFTER(eh, behavior_sticky_key);
                    release_sticky_key_behavior(sticky_key, ev->timestamp);
                    return ZMK_EV_EVENT_CAPTURED;
                }
            }
            sticky_key->modified_key_usage_page = ev->usage_page;
            sticky_key->modified_key_keycode = ev->keycode;
        } else { // key up
            if (sticky_key->timer_started &&
                sticky_key->modified_key_usage_page == ev->usage_page &&
                sticky_key->modified_key_keycode == ev->keycode) {
                stop_timer(sticky_key);
                release_sticky_key_behavior(sticky_key, ev->timestamp);
            }
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

void behavior_sticky_key_timer_handler(struct k_work *item) {
    struct active_sticky_key *sticky_key =
        CONTAINER_OF(item, struct active_sticky_key, release_timer);
    if (sticky_key->position == ZMK_BHV_STICKY_KEY_POSITION_FREE) {
        return;
    }
    if (sticky_key->timer_cancelled) {
        sticky_key->timer_cancelled = false;
    } else {
        release_sticky_key_behavior(sticky_key, sticky_key->release_at);
    }
}

static int behavior_sticky_key_init(const struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
            k_delayed_work_init(&active_sticky_keys[i].release_timer,
                                behavior_sticky_key_timer_handler);
            active_sticky_keys[i].position = ZMK_BHV_STICKY_KEY_POSITION_FREE;
        }
    }
    init_first_run = false;
    return 0;
}

struct behavior_sticky_key_data {};
static struct behavior_sticky_key_data behavior_sticky_key_data;

#define KP_INST(n)                                                                                 \
    static struct behavior_sticky_key_config behavior_sticky_key_config_##n = {                    \
        .behavior = ZMK_KEYMAP_EXTRACT_BINDING(0, DT_DRV_INST(n)),                                 \
        .release_after_ms = DT_INST_PROP(n, release_after_ms),                                     \
        .quick_release = DT_INST_PROP(n, quick_release),                                           \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, behavior_sticky_key_init, device_pm_control_nop,                      \
                          &behavior_sticky_key_data, &behavior_sticky_key_config_##n, APPLICATION, \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_sticky_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
