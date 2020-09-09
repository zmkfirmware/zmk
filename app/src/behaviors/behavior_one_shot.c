/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_one_shot

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

#define ZMK_BHV_ONE_SHOT_MAX_HELD 10

// increase this if you need more keys in the board
#define ZMK_BHV_ONE_SHOT_POSITION_NOT_USED 9999

struct behavior_one_shot_config {
    s32_t release_after_ms;
    struct zmk_behavior_binding behavior;
};

// this data is specific for each hold-tap
struct active_one_shot {
    s32_t position;
    s32_t param1;
    s32_t param2;
    const struct behavior_one_shot_config *config;
    // timer data.
    s32_t release_at;
    struct k_delayed_work release_after_timer;
    bool timer_is_cancelled;
    // usage page and keycode for the key that is being modified by this one shot
    u32_t modified_key_position;
};

struct active_one_shot active_one_shots[ZMK_BHV_ONE_SHOT_MAX_HELD] = {};

static struct active_one_shot *store_one_shot(u32_t position, u32_t param1, u32_t param2,
                                              const struct behavior_one_shot_config *config) {
    for (int i = 0; i < ZMK_BHV_ONE_SHOT_MAX_HELD; i++) {
        if (active_one_shots[i].position != ZMK_BHV_ONE_SHOT_POSITION_NOT_USED) {
            continue;
        }
        active_one_shots[i].position = position;
        active_one_shots[i].param1 = param1;
        active_one_shots[i].param2 = param2;
        active_one_shots[i].config = config;
        active_one_shots[i].release_at = 0;
        active_one_shots[i].timer_is_cancelled = false;
        active_one_shots[i].modified_key_position = ZMK_BHV_ONE_SHOT_POSITION_NOT_USED;
        return &active_one_shots[i];
    }
    return NULL;
}

static void clear_one_shot(struct active_one_shot *one_shot) {
    one_shot->position = ZMK_BHV_ONE_SHOT_POSITION_NOT_USED;
}

static struct active_one_shot *find_one_shot(u32_t position) {
    for (int i = 0; i < ZMK_BHV_ONE_SHOT_MAX_HELD; i++) {
        if (active_one_shots[i].position == position) {
            return &active_one_shots[i];
        }
    }
    return NULL;
}

static inline int press_one_shot_behavior(struct active_one_shot *one_shot, s32_t timestamp) {
    const struct zmk_behavior_binding *behavior = &one_shot->config->behavior;
    struct device *behavior_device = device_get_binding(behavior->behavior_dev);
    return behavior_keymap_binding_pressed(behavior_device, one_shot->position, one_shot->param1,
                                           one_shot->param2, timestamp);
}

static inline int release_one_shot_behavior(struct active_one_shot *one_shot, s32_t timestamp) {
    const struct zmk_behavior_binding *behavior = &one_shot->config->behavior;
    struct device *behavior_device = device_get_binding(behavior->behavior_dev);
    return behavior_keymap_binding_released(behavior_device, one_shot->position, one_shot->param1,
                                            one_shot->param2, timestamp);
}

static int stop_timer(struct active_one_shot *one_shot) {
    int timer_cancel_result = k_delayed_work_cancel(&one_shot->release_after_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        one_shot->timer_is_cancelled = true;
    }
    return timer_cancel_result;
}

static int on_one_shot_binding_pressed(struct device *dev, u32_t position, u32_t param1,
                                       u32_t param2, s64_t timestamp) {
    const struct behavior_one_shot_config *cfg = dev->config_info;

    struct active_one_shot *one_shot = store_one_shot(position, param1, param2, cfg);
    if (one_shot == NULL) {
        LOG_ERR("unable to store one-shot info, did you press more than %d one_shots?",
                ZMK_BHV_ONE_SHOT_MAX_HELD);
        return 0;
    }

    press_one_shot_behavior(one_shot, timestamp);
    LOG_DBG("%d new one_shot", position);
    return 0;
}

static int on_one_shot_binding_released(struct device *dev, u32_t position, u32_t _, u32_t __,
                                        s64_t timestamp) {
    struct active_one_shot *one_shot = find_one_shot(position);
    if (one_shot == NULL) {
        LOG_ERR("ACTIVE ONE SHOT CLEARED TOO EARLY");
        return 0;
    }

    if (one_shot->modified_key_position != ZMK_BHV_ONE_SHOT_POSITION_NOT_USED) {
        // Another key was pressed while the one-shot was pressed. Act like a normal key.
        int retval = release_one_shot_behavior(one_shot, timestamp);
        clear_one_shot(one_shot);
        return retval;
    }

    // No other key was pressed. Start the timer.
    one_shot->release_at = timestamp + one_shot->config->release_after_ms;
    // adjust timer in case this behavior was queued by a hold-tap
    s32_t ms_left = one_shot->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_delayed_work_submit(&one_shot->release_after_timer, K_MSEC(ms_left));
    }
    return 0;
}

static const struct behavior_driver_api behavior_one_shot_driver_api = {
    .binding_pressed = on_one_shot_binding_pressed,
    .binding_released = on_one_shot_binding_released,
};

static int one_shot_keycode_state_changed_listener(const struct zmk_event_header *eh) {
    if (!is_keycode_state_changed(eh)) {
        return 0;
    }
    struct keycode_state_changed *ev = cast_keycode_state_changed(eh);
    for (int i = 0; i < ZMK_BHV_ONE_SHOT_MAX_HELD; i++) {
        struct active_one_shot *one_shot = &active_one_shots[i];
        if (one_shot->position == ZMK_BHV_ONE_SHOT_POSITION_NOT_USED ||
            one_shot->position == ev->position) {
            continue;
        }
        // If events were queued, the timer event may be queued late or not at all.
        // Release the one-shot if the timer should've run out in the meantime.
        if (one_shot->release_at != 0 && ev->timestamp > one_shot->release_at) {
            release_one_shot_behavior(one_shot, one_shot->release_at);
            if (stop_timer(one_shot)) {
                clear_one_shot(one_shot);
            }
            continue;
        }

        if (ev->state) { // key down
            if (one_shot->modified_key_position != ZMK_BHV_ONE_SHOT_POSITION_NOT_USED) {
                continue;
            }
            one_shot->modified_key_position = ev->position;
            if (one_shot->release_at) {
                stop_timer(one_shot);
            }
        } else { // key up
            if (one_shot->modified_key_position != ev->position || one_shot->release_at == 0) {
                continue;
            }
            release_one_shot_behavior(one_shot, ev->timestamp);
        }
    }
    return 0;
}

ZMK_LISTENER(behavior_one_shot, one_shot_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_one_shot, keycode_state_changed);

void behavior_one_shot_timer_handler(struct k_work *item) {
    struct active_one_shot *one_shot =
        CONTAINER_OF(item, struct active_one_shot, release_after_timer);
    if (one_shot->position == ZMK_BHV_ONE_SHOT_POSITION_NOT_USED) {
        return;
    }
    if (!one_shot->timer_is_cancelled) {
        release_one_shot_behavior(one_shot, k_uptime_get());
    }
    clear_one_shot(one_shot);
}

static int behavior_one_shot_init(struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_ONE_SHOT_MAX_HELD; i++) {
            k_delayed_work_init(&active_one_shots[i].release_after_timer,
                                behavior_one_shot_timer_handler);
            active_one_shots[i].position = ZMK_BHV_ONE_SHOT_POSITION_NOT_USED;
        }
    }
    init_first_run = false;
    return 0;
}

struct behavior_one_shot_data {};
static struct behavior_one_shot_data behavior_one_shot_data;

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    },

#define KP_INST(n)                                                                                 \
    static struct behavior_one_shot_config behavior_one_shot_config_##n = {                        \
        .behavior = _TRANSFORM_ENTRY(0, n).release_after_ms = DT_INST_PROP(n, release_after_ms),   \
    };                                                                                             \
    DEVICE_AND_API_INIT(behavior_one_shot_##n, DT_INST_LABEL(n), behavior_one_shot_init,           \
                        &behavior_one_shot_data, &behavior_one_shot_config_##n, APPLICATION,       \
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_one_shot_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif