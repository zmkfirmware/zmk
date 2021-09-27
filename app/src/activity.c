/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <power/power.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>

#include <zmk/activity.h>

static enum zmk_activity_state activity_state;

static uint32_t activity_last_uptime;

#define MAX_IDLE_MS CONFIG_ZMK_IDLE_TIMEOUT

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
#define MAX_SLEEP_MS CONFIG_ZMK_IDLE_SLEEP_TIMEOUT
#endif

int raise_event() {
    return ZMK_EVENT_RAISE(new_zmk_activity_state_changed(
        (struct zmk_activity_state_changed){.state = activity_state}));
}

int set_state(enum zmk_activity_state state) {
    if (activity_state == state)
        return 0;

    activity_state = state;
    return raise_event();
}

enum zmk_activity_state zmk_activity_get_state() { return activity_state; }

int activity_event_listener(const zmk_event_t *eh) {
    activity_last_uptime = k_uptime_get();

    return set_state(ZMK_ACTIVITY_ACTIVE);
}

void activity_work_handler(struct k_work *work) {
    int32_t current = k_uptime_get();
    int32_t inactive_time = current - activity_last_uptime;
#if IS_ENABLED(CONFIG_ZMK_SLEEP)
    if (inactive_time > MAX_SLEEP_MS) {
        // Put devices in low power mode before sleeping
        pm_power_state_force((struct pm_state_info){PM_STATE_STANDBY, 0, 0});
        set_state(ZMK_ACTIVITY_SLEEP);
    } else
#endif /* IS_ENABLED(CONFIG_ZMK_SLEEP) */
        if (inactive_time > MAX_IDLE_MS) {
        set_state(ZMK_ACTIVITY_IDLE);
    }
}

K_WORK_DEFINE(activity_work, activity_work_handler);

void activity_expiry_function() { k_work_submit(&activity_work); }

K_TIMER_DEFINE(activity_timer, activity_expiry_function, NULL);

int activity_init() {
    activity_last_uptime = k_uptime_get();

    k_timer_start(&activity_timer, K_SECONDS(1), K_SECONDS(1));
    return 0;
}

ZMK_LISTENER(activity, activity_event_listener);
ZMK_SUBSCRIPTION(activity, zmk_position_state_changed);
ZMK_SUBSCRIPTION(activity, zmk_sensor_event);

SYS_INIT(activity_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
