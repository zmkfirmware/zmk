/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/events/sync_activity_event.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC) ||                             \
    IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_ON_EVENT)
#include <zmk/split/bluetooth/central.h>
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC) ||
       // IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_ON_EVENT)

#include <zmk/pm.h>

#include <zmk/activity.h>

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/usb.h>
#endif

bool is_usb_power_present(void) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    return zmk_usb_is_powered();
#else
    return false;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
}

static enum zmk_activity_state activity_state;

static uint32_t activity_last_uptime;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC)
static uint32_t last_periodic_sync_time;
#define PERIODIC_SYNC_INTERVAL_MS CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC_INTERVAL_MS
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_ON_EVENT)
static uint32_t last_event_sync_time;
#define EVENT_SYNC_MIN_INTERVAL_MS CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_EVENT_MIN_INTERVAL
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_ON_EVENT)

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#define MAX_IDLE_MS CONFIG_ZMK_IDLE_TIMEOUT

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
#define MAX_SLEEP_MS CONFIG_ZMK_IDLE_SLEEP_TIMEOUT
#endif

int raise_event(void) {
    return raise_zmk_activity_state_changed(
        (struct zmk_activity_state_changed){.state = activity_state});
}

int set_state(enum zmk_activity_state state) {
    if (activity_state == state)
        return 0;

    activity_state = state;
    return raise_event();
}

enum zmk_activity_state zmk_activity_get_state(void) { return activity_state; }

int activity_event_listener(const zmk_event_t *eh) {
    activity_last_uptime = k_uptime_get();

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) &&                                                   \
    IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_ON_EVENT)
    if (activity_last_uptime - last_event_sync_time > EVENT_SYNC_MIN_INTERVAL_MS) {
        LOG_DBG("Refresh %d", activity_last_uptime - last_event_sync_time);
        last_event_sync_time = activity_last_uptime;
        zmk_split_bt_queue_sync_activity(0);
    }
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) &&
       // IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_ON_EVENT)

    return set_state(ZMK_ACTIVITY_ACTIVE);
}

void activity_work_handler(struct k_work *work) {
    int32_t current = k_uptime_get();
    int32_t inactive_time = current - activity_last_uptime;
#if IS_ENABLED(CONFIG_ZMK_SLEEP)
    if (inactive_time > MAX_SLEEP_MS && !is_usb_power_present()) {
        // Put devices in suspend power mode before sleeping
        set_state(ZMK_ACTIVITY_SLEEP);

        if (zmk_pm_suspend_devices() < 0) {
            LOG_ERR("Failed to suspend all the devices");
            zmk_pm_resume_devices();
            return;
        }

        sys_poweroff();
    } else
#endif /* IS_ENABLED(CONFIG_ZMK_SLEEP) */
        if (inactive_time > MAX_IDLE_MS) {
            set_state(ZMK_ACTIVITY_IDLE);
        }

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) &&                                                   \
    IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC)
    if (current - last_periodic_sync_time > PERIODIC_SYNC_INTERVAL_MS) {
        last_periodic_sync_time = current;
        zmk_split_bt_queue_sync_activity(inactive_time);
    }
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) &&
       // IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC)
}

K_WORK_DEFINE(activity_work, activity_work_handler);

void activity_expiry_function(struct k_timer *_timer) { k_work_submit(&activity_work); }

K_TIMER_DEFINE(activity_timer, activity_expiry_function, NULL);

static int activity_init(void) {
    activity_last_uptime = k_uptime_get();

    k_timer_start(&activity_timer, K_SECONDS(1), K_SECONDS(1));
    return 0;
}

ZMK_LISTENER(activity, activity_event_listener);
ZMK_SUBSCRIPTION(activity, zmk_position_state_changed);
ZMK_SUBSCRIPTION(activity, zmk_sensor_event);

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) &&                                                  \
    (IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC) ||                            \
     IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_ON_EVENT))

int sync_activity_event_listener(const zmk_event_t *eh) {
    int32_t current = k_uptime_get();

    struct zmk_sync_activity_event *ev = as_zmk_sync_activity_event(eh);
    if (ev == NULL) {
        LOG_ERR("Invalid event type");
        return -ENOTSUP;
    }

    activity_last_uptime = current - ev->central_inactive_duration;

    if (activity_state == ZMK_ACTIVITY_IDLE && ev->central_inactive_duration < MAX_IDLE_MS) {
        LOG_DBG("Syncing state to active to match central device.");
        return set_state(ZMK_ACTIVITY_ACTIVE);
    }
    return 0;
}

ZMK_LISTENER(sync_activity, sync_activity_event_listener);
ZMK_SUBSCRIPTION(sync_activity, zmk_sync_activity_event);
#endif // !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) &&
       // (IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_PERIODIC) ||
       // IS_ENABLED(CONFIG_ZMK_SPLIT_SYNC_LAST_ACTIVITY_TIMING_ON_EVENT))

SYS_INIT(activity_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
