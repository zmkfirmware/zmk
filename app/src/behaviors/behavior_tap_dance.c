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
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ZMK_BHV_TAP_DANCE_POSITION_FREE ULONG_MAX

struct behavior_tap_dance_config {    
    uint32_t tapping_term_ms;
    struct zmk_behavior_binding behavior;
};

struct active_tap_dance {
    // tap dance data
    int counter;
    uint32_t position;
    const struct behavior_tap_dance_config *config;
    // timer data
    bool timer_started;
    bool timer_cancelled;
    int64_t release_at;
    struct k_delayed_work release_timer;
};

struct active_tap_dance the_tap_dance;

static void clear_tap_dance(struct active_tap_dance *tap_dance) {
    LOG_DBG("Clearing Tap Dance");
    tap_dance-> position = ZMK_BHV_TAP_DANCE_POSITION_FREE;
}



static inline int press_tap_dance_behavior(struct active_tap_dance *tap_dance,
                                            int64_t timestamp) {
    LOG_DBG("Press Tap Dance Behavior");
    struct zmk_behavior_binding binding = {
        .behavior_dev = tap_dance->config->behavior.behavior_dev,
    };
    struct zmk_behavior_binding_event event = {
        .position = tap_dance->position,
        .timestamp = timestamp,
    };
    return 0;
    //return behavior_keymap_binding_pressed(&binding, event);
}

static inline int release_tap_dance_behavior(struct active_tap_dance *tap_dance,
                                              int64_t timestamp) {
    LOG_DBG("Release Tap Dance Behavior");
    struct zmk_behavior_binding binding = {
        .behavior_dev = tap_dance->config->behavior.behavior_dev,
    };
    struct zmk_behavior_binding_event event = {
        .position = tap_dance->position,
        .timestamp = timestamp,
    };

    clear_tap_dance(tap_dance);
    return 0;
    //return behavior_keymap_binding_released(&binding, event);
}

/*static int stop_timer(struct active_tap_dance *tap_dance) {
    LOG_DBG("Stop Timer");
    int timer_cancel_result = k_delayed_work_cancel(&tap_dance->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        tap_dance->timer_cancelled = true;
    }
    return timer_cancel_result;

    static struct active_tap_dance  *tap_dance
}*/



static int on_tap_dance_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    LOG_DBG("On Binding Pressed");
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_tap_dance_config *cfg = dev->config;

    if (!the_tap_dance.timer_started){
        LOG_DBG("Initializing Tap Dance");
        the_tap_dance.timer_started = true;
        the_tap_dance.counter = 1;
        the_tap_dance.position = event.position;
        the_tap_dance.config = cfg;
        the_tap_dance.release_at = 0;
        the_tap_dance.timer_cancelled = false;
        the_tap_dance.timer_started = false;
    }
    else {
        LOG_DBG("Incrementing Tap Dance");
        the_tap_dance.counter ++;
    }
    LOG_DBG("Counter is now at value: %d", the_tap_dance.counter);
    return ZMK_BEHAVIOR_OPAQUE;    
}

static int on_tap_dance_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("On Binding Released");
    struct active_tap_dance *tap_dance = &the_tap_dance;
    // No other key was pressed. Start the timer.
    tap_dance->timer_started = true;
    tap_dance->release_at = event.timestamp + tap_dance->config->tapping_term_ms;
    // adjust timer in case this behavior was queued by a hold-tap
    int32_t ms_left = tap_dance->release_at - k_uptime_get();
    LOG_DBG("ms_left equal to: %d", ms_left);
    if (ms_left > 0) {
        k_delayed_work_submit(&tap_dance->release_timer, K_MSEC(ms_left));
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_tap_dance_driver_api = {
    .binding_pressed = on_tap_dance_binding_pressed,
    .binding_released = on_tap_dance_binding_released,
};

void behavior_tap_dance_timer_handler(struct k_work *item) {
    LOG_DBG("Timer Handler Called");
    struct active_tap_dance *tap_dance =
        CONTAINER_OF(item, struct active_tap_dance, release_timer);
    if (tap_dance->position == ZMK_BHV_TAP_DANCE_POSITION_FREE) {
        return;
    }
    if (tap_dance->timer_cancelled) {
        tap_dance->timer_cancelled = false;
    } else {
        release_tap_dance_behavior(tap_dance, tap_dance->release_at);
    }
    the_tap_dance.counter = 0;
    the_tap_dance.timer_started = false;
}

static int behavior_tap_dance_init(const struct device *dev) { 
    static bool init_first_run = true;
    if (init_first_run) {
        k_delayed_work_init(&the_tap_dance.release_timer,
                            behavior_tap_dance_timer_handler);
        LOG_DBG("Hello World");
        the_tap_dance.position = ZMK_BHV_TAP_DANCE_POSITION_FREE;
    }
    init_first_run = false;
    return 0;
}

struct behavior_tap_dance_data {};
static struct behavior_tap_dance_data behavior_tap_dance_data;

#define KP_INST(n)                                                                                                         \
    static struct behavior_tap_dance_config behavior_tap_dance_config_##n = {                                              \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                                                                                           \
    };                                                                                                                     \
    DEVICE_AND_API_INIT(behavior_tap_dance_##n, DT_INST_LABEL(n), behavior_tap_dance_init, &behavior_tap_dance_data,       \
                        &behavior_tap_dance_config_##n, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                        &behavior_tap_dance_driver_api);
    
DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif