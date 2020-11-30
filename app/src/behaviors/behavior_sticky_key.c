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
#include <zmk/event-manager.h>
#include <zmk/events/position-state-changed.h>
#include <zmk/events/keycode-state-changed.h>
#include <zmk/events/modifiers-state-changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_NODE_EXISTS(DT_DRV_INST(0))

#define ZMK_BHV_STICKY_KEY_MAX_HELD 10

// increase this if you need more keys in the board
#define ZMK_BHV_STICKY_KEY_POSITION_NOT_USED ULONG_MAX

struct behavior_sticky_key_config {
    u32_t release_after_ms;
    struct zmk_behavior_binding behavior;
};

struct active_sticky_key {
    u32_t position;
    u32_t param1;
    u32_t param2;
    const struct behavior_sticky_key_config *config;
    // timer data.
    bool timer_started;
    s64_t release_at;
    struct k_delayed_work release_timer;
    bool timer_is_cancelled;
    // usage page and keycode for the key that is being modified by this sticky key
    u8_t modified_key_usage_page;
    u32_t modified_key_keycode;
};

struct active_sticky_key active_sticky_keys[ZMK_BHV_STICKY_KEY_MAX_HELD] = {};

static struct active_sticky_key *store_sticky_key(u32_t position, u32_t param1, u32_t param2,
                                                  const struct behavior_sticky_key_config *config) {
    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        if (active_sticky_keys[i].position != ZMK_BHV_STICKY_KEY_POSITION_NOT_USED ||
            active_sticky_keys[i].timer_is_cancelled) {
            continue;
        }
        active_sticky_keys[i].position = position;
        active_sticky_keys[i].param1 = param1;
        active_sticky_keys[i].param2 = param2;
        active_sticky_keys[i].config = config;
        active_sticky_keys[i].release_at = 0;
        active_sticky_keys[i].timer_is_cancelled = false;
        active_sticky_keys[i].timer_started = false;
        active_sticky_keys[i].modified_key_usage_page = 0;
        active_sticky_keys[i].modified_key_keycode = 0;
        return &active_sticky_keys[i];
    }
    return NULL;
}

static void clear_sticky_key(struct active_sticky_key *sticky_key) {
    sticky_key->position = ZMK_BHV_STICKY_KEY_POSITION_NOT_USED;
}

static struct active_sticky_key *find_sticky_key(u32_t position) {
    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        if (active_sticky_keys[i].position == position &&
            !active_sticky_keys[i].timer_is_cancelled) {
            return &active_sticky_keys[i];
        }
    }
    return NULL;
}

static inline int press_sticky_key_behavior(struct active_sticky_key *sticky_key, s64_t timestamp) {
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
                                              s64_t timestamp) {
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
        sticky_key->timer_is_cancelled = true;
    }
    return timer_cancel_result;
}

static int on_sticky_key_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sticky_key_config *cfg = dev->config_info;
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
        return 0;
    }

    press_sticky_key_behavior(sticky_key, event.timestamp);
    LOG_DBG("%d new sticky_key", event.position);
    return 0;
}

static int on_sticky_key_binding_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    struct active_sticky_key *sticky_key = find_sticky_key(event.position);
    if (sticky_key == NULL) {
        LOG_ERR("ACTIVE STICKY KEY CLEARED TOO EARLY");
        return 0;
    }

    if (sticky_key->modified_key_usage_page != 0 && sticky_key->modified_key_keycode != 0) {
        LOG_DBG("Another key was pressed while the sticky key was pressed. Act like a normal key.");
        return release_sticky_key_behavior(sticky_key, event.timestamp);
    }

    // No other key was pressed. Start the timer.
    sticky_key->timer_started = true;
    sticky_key->release_at = event.timestamp + sticky_key->config->release_after_ms;
    // adjust timer in case this behavior was queued by a hold-tap
    s32_t ms_left = sticky_key->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_delayed_work_submit(&sticky_key->release_timer, K_MSEC(ms_left));
    }
    return 0;
}

static const struct behavior_driver_api behavior_sticky_key_driver_api = {
    .binding_pressed = on_sticky_key_binding_pressed,
    .binding_released = on_sticky_key_binding_released,
};

static int sticky_key_keycode_state_changed_listener(const struct zmk_event_header *eh) {
    if (!is_keycode_state_changed(eh)) {
        return 0;
    }
    struct keycode_state_changed *ev = cast_keycode_state_changed(eh);
    for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        struct active_sticky_key *sticky_key = &active_sticky_keys[i];
        if (sticky_key->position == ZMK_BHV_STICKY_KEY_POSITION_NOT_USED) {
            continue;
        }

        if (strcmp(sticky_key->config->behavior.behavior_dev, "KEY_PRESS") == 0 &&
            HID_USAGE_ID(sticky_key->param1) == ev->keycode &&
            HID_USAGE_PAGE(sticky_key->param1) == ev->usage_page &&
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
    return 0;
}

ZMK_LISTENER(behavior_sticky_key, sticky_key_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_sticky_key, keycode_state_changed);

void behavior_sticky_key_timer_handler(struct k_work *item) {
    struct active_sticky_key *sticky_key =
        CONTAINER_OF(item, struct active_sticky_key, release_timer);
    if (sticky_key->position == ZMK_BHV_STICKY_KEY_POSITION_NOT_USED) {
        return;
    }
    if (sticky_key->timer_is_cancelled) {
        sticky_key->timer_is_cancelled = false;
    } else {
        release_sticky_key_behavior(sticky_key, sticky_key->release_at);
    }
}

static int behavior_sticky_key_init(struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
            k_delayed_work_init(&active_sticky_keys[i].release_timer,
                                behavior_sticky_key_timer_handler);
            active_sticky_keys[i].position = ZMK_BHV_STICKY_KEY_POSITION_NOT_USED;
        }
    }
    init_first_run = false;
    return 0;
}

struct behavior_sticky_key_data {};
static struct behavior_sticky_key_data behavior_sticky_key_data;

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    },

#define KP_INST(n)                                                                                 \
    static struct behavior_sticky_key_config behavior_sticky_key_config_##n = {                    \
        .behavior = _TRANSFORM_ENTRY(0, n).release_after_ms = DT_INST_PROP(n, release_after_ms),   \
    };                                                                                             \
    DEVICE_AND_API_INIT(behavior_sticky_key_##n, DT_INST_LABEL(n), behavior_sticky_key_init,       \
                        &behavior_sticky_key_data, &behavior_sticky_key_config_##n, APPLICATION,   \
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_sticky_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif