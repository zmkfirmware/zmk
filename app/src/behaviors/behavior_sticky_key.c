/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sticky_key

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
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

#define KEY_PRESS DEVICE_DT_NAME(DT_INST(0, zmk_behavior_key_press))

#define ZMK_BHV_STICKY_KEY_MAX_HELD 10

#define ZMK_BHV_STICKY_KEY_POSITION_FREE UINT32_MAX

struct behavior_sticky_key_config {
    uint32_t release_after_ms;
    bool quick_release;
    bool lazy;
    bool ignore_modifiers;
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
    struct k_work_delayable release_timer;
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

static inline void on_sticky_key_timeout(struct active_sticky_key *sticky_key) {
    // If the key is lazy, a release is not needed on timeout
    if (sticky_key->config->lazy) {
        clear_sticky_key(sticky_key);
    } else {
        release_sticky_key_behavior(sticky_key, sticky_key->release_at);
    }
}

static int stop_timer(struct active_sticky_key *sticky_key) {
    int timer_cancel_result = k_work_cancel_delayable(&sticky_key->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        sticky_key->timer_cancelled = true;
    }
    return timer_cancel_result;
}

static int on_sticky_key_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
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

    LOG_DBG("%d new sticky_key", event.position);
    if (!sticky_key->config->lazy) {
        // press the key now if it's not lazy
        press_sticky_key_behavior(sticky_key, event.timestamp);
    }
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
        k_work_schedule(&sticky_key->release_timer, K_MSEC(ms_left));
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int sticky_key_parameter_domains(const struct device *sk,
                                        struct behavior_parameter_metadata *param_metadata) {
    const struct behavior_sticky_key_config *cfg = sk->config;

    struct behavior_parameter_metadata child_metadata;

    int err = behavior_get_parameter_metadata(zmk_behavior_get_binding(cfg->behavior.behavior_dev),
                                              &child_metadata);
    if (err < 0) {
        LOG_WRN("Failed to get the sticky key bound behavior parameter: %d", err);
    }

    for (int s = 0; s < child_metadata.sets_len; s++) {
        const struct behavior_parameter_metadata_set *set = &child_metadata.sets[s];

        if (set->param2_values_len > 0) {
            return -ENOTSUP;
        }
    }

    *param_metadata = child_metadata;

    return 0;
}

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_driver_api behavior_sticky_key_driver_api = {
    .binding_pressed = on_sticky_key_binding_pressed,
    .binding_released = on_sticky_key_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = sticky_key_parameter_domains,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

static int sticky_key_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_sticky_key, sticky_key_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_sticky_key, zmk_keycode_state_changed);

static int sticky_key_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // we want to make sure every sticky key is given a chance to lazy press their behavior before
    // the event gets reraised, and release their behavior after the event is reraised, so we keep
    // track of them. this allows us to ensure the sticky key is pressed and released "around" the
    // other key, especially in the case of lazy keys.
    struct active_sticky_key *sticky_keys_to_press_before_reraise[ZMK_BHV_STICKY_KEY_MAX_HELD];
    struct active_sticky_key *sticky_keys_to_release_after_reraise[ZMK_BHV_STICKY_KEY_MAX_HELD];

    // reraising the event frees it, so make a copy of any event data we might
    // need after it's been freed.
    const struct zmk_keycode_state_changed ev_copy = *ev;

    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        sticky_keys_to_press_before_reraise[i] = NULL;
        sticky_keys_to_release_after_reraise[i] = NULL;

        struct active_sticky_key *sticky_key = &active_sticky_keys[i];
        if (sticky_key->position == ZMK_BHV_STICKY_KEY_POSITION_FREE) {
            continue;
        }

        if (strcmp(sticky_key->config->behavior.behavior_dev, KEY_PRESS) == 0 &&
            ZMK_HID_USAGE_ID(sticky_key->param1) == ev_copy.keycode &&
            ZMK_HID_USAGE_PAGE(sticky_key->param1) == ev_copy.usage_page &&
            SELECT_MODS(sticky_key->param1) == ev_copy.implicit_modifiers) {
            // don't catch key down events generated by the sticky key behavior itself
            continue;
        }

        if (ev_copy.state) { // key down
            if (sticky_key->config->ignore_modifiers &&
                is_mod(ev_copy.usage_page, ev_copy.keycode)) {
                // ignore modifier key press so we can stack sticky keys and combine with other
                // modifiers
                continue;
            }
            if (sticky_key->modified_key_usage_page != 0 || sticky_key->modified_key_keycode != 0) {
                // this sticky key is already in use for a keycode
                continue;
            }

            // we don't want the timer to release the sticky key before the other key is released
            stop_timer(sticky_key);

            // If this event was queued, the timer may be triggered late or not at all.
            // Release the sticky key if the timer should've run out in the meantime.
            if (sticky_key->release_at != 0 && ev_copy.timestamp > sticky_key->release_at) {
                on_sticky_key_timeout(sticky_key);
                continue;
            }

            if (sticky_key->config->lazy) {
                // if the sticky key is lazy, we need to press it before the event is reraised
                sticky_keys_to_press_before_reraise[i] = sticky_key;
            }
            if (sticky_key->timer_started) {
                if (sticky_key->config->quick_release) {
                    // immediately release the sticky key after the key press is handled.
                    sticky_keys_to_release_after_reraise[i] = sticky_key;
                }
            }
            sticky_key->modified_key_usage_page = ev_copy.usage_page;
            sticky_key->modified_key_keycode = ev_copy.keycode;
        } else { // key up
            if (sticky_key->timer_started &&
                sticky_key->modified_key_usage_page == ev_copy.usage_page &&
                sticky_key->modified_key_keycode == ev_copy.keycode) {
                stop_timer(sticky_key);
                sticky_keys_to_release_after_reraise[i] = sticky_key;
            }
        }
    }

    // give each sticky key a chance to press their behavior before the event is reraised
    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        struct active_sticky_key *sticky_key = sticky_keys_to_press_before_reraise[i];
        if (!sticky_key) {
            continue;
        }

        press_sticky_key_behavior(sticky_key, ev_copy.timestamp);
    }
    // give each sticky key a chance to release their behavior after the event is reraised, lazily
    // reraising. keep track whether the event has been reraised so we only reraise it once
    bool event_reraised = false;
    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        struct active_sticky_key *sticky_key = sticky_keys_to_release_after_reraise[i];
        if (!sticky_key) {
            continue;
        }

        if (!event_reraised) {
            struct zmk_keycode_state_changed_event dupe_ev =
                copy_raised_zmk_keycode_state_changed(ev);
            ZMK_EVENT_RAISE_AFTER(dupe_ev, behavior_sticky_key);
            event_reraised = true;
        }
        release_sticky_key_behavior(sticky_key, ev_copy.timestamp);
    }

    return event_reraised ? ZMK_EV_EVENT_CAPTURED : ZMK_EV_EVENT_BUBBLE;
}

void behavior_sticky_key_timer_handler(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);
    struct active_sticky_key *sticky_key =
        CONTAINER_OF(d_work, struct active_sticky_key, release_timer);
    if (sticky_key->position == ZMK_BHV_STICKY_KEY_POSITION_FREE) {
        return;
    }
    if (sticky_key->timer_cancelled) {
        sticky_key->timer_cancelled = false;
    } else {
        on_sticky_key_timeout(sticky_key);
    }
}

static int behavior_sticky_key_init(const struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
            k_work_init_delayable(&active_sticky_keys[i].release_timer,
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
    static const struct behavior_sticky_key_config behavior_sticky_key_config_##n = {              \
        .behavior = ZMK_KEYMAP_EXTRACT_BINDING(0, DT_DRV_INST(n)),                                 \
        .release_after_ms = DT_INST_PROP(n, release_after_ms),                                     \
        .quick_release = DT_INST_PROP(n, quick_release),                                           \
        .lazy = DT_INST_PROP(n, lazy),                                                             \
        .ignore_modifiers = DT_INST_PROP(n, ignore_modifiers),                                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_sticky_key_init, NULL, &behavior_sticky_key_data,          \
                            &behavior_sticky_key_config_##n, POST_KERNEL,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_sticky_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
