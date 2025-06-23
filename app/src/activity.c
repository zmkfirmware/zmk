/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>

#include <zmk/pm.h>

#include <zmk/activity.h>

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/usb.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_POINTING)
#include <zephyr/input/input.h>
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

static uint32_t idle_timeout_ms = CONFIG_ZMK_IDLE_TIMEOUT;

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
static uint32_t sleep_timeout_ms = CONFIG_ZMK_IDLE_SLEEP_TIMEOUT;
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

#if IS_ENABLED(CONFIG_SETTINGS)
static void activity_save_timeouts_work(struct k_work *work) {
    settings_save_one("activity/idle", &idle_timeout_ms, sizeof(idle_timeout_ms));
#if IS_ENABLED(CONFIG_ZMK_SLEEP)
    settings_save_one("activity/sleep", &sleep_timeout_ms, sizeof(sleep_timeout_ms));
#endif
}

static struct k_work_delayable activity_save_work;
#endif

static int activity_save_timeouts(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    return k_work_reschedule(&activity_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#else
    return 0;
#endif
}

uint32_t zmk_activity_get_idle_timeout(void) { return idle_timeout_ms; }
void zmk_activity_set_idle_timeout(uint32_t timeout) {
    idle_timeout_ms = timeout;
    activity_save_timeouts();
}

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
uint32_t zmk_activity_get_sleep_timeout(void) { return sleep_timeout_ms; }
void zmk_activity_set_sleep_timeout(uint32_t timeout) {
    sleep_timeout_ms = timeout;
    activity_save_timeouts();
}
#endif

static int note_activity(void) {
    activity_last_uptime = k_uptime_get();

    return set_state(ZMK_ACTIVITY_ACTIVE);
}

static int activity_event_listener(const zmk_event_t *eh) { return note_activity(); }

void activity_work_handler(struct k_work *work) {
    int32_t current = k_uptime_get();
    int32_t inactive_time = current - activity_last_uptime;
#if IS_ENABLED(CONFIG_ZMK_SLEEP)
    if (inactive_time > sleep_timeout_ms && !is_usb_power_present()) {
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
        if (inactive_time > idle_timeout_ms) {
            set_state(ZMK_ACTIVITY_IDLE);
        }
}

K_WORK_DEFINE(activity_work, activity_work_handler);

void activity_expiry_function(struct k_timer *_timer) { k_work_submit(&activity_work); }

K_TIMER_DEFINE(activity_timer, activity_expiry_function, NULL);

#if IS_ENABLED(CONFIG_SETTINGS)

static int activity_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                               void *cb_arg) {
    LOG_DBG("Setting timeout value %s", name);

    if (settings_name_steq(name, "idle", NULL)) {
        if (len != sizeof(idle_timeout_ms)) {
            LOG_ERR("Invalid timeout size (got %d expected %d)", len, sizeof(idle_timeout_ms));
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &idle_timeout_ms, sizeof(idle_timeout_ms));
        if (err <= 0) {
            LOG_ERR("Failed to read idle timeout from settings (err %d)", err);
            return err;
        }
    }
#if IS_ENABLED(CONFIG_ZMK_SLEEP)
    else if (settings_name_steq(name, "sleep", NULL)) {
        if (len != sizeof(sleep_timeout_ms)) {
            LOG_ERR("Invalid timeout size (got %d expected %d)", len, sizeof(sleep_timeout_ms));
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &sleep_timeout_ms, sizeof(sleep_timeout_ms));
        if (err <= 0) {
            LOG_ERR("Failed to read sleep timeout from settings (err %d)", err);
            return err;
        }
    }
#endif

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(activity, "activity", NULL, activity_handle_set, NULL, NULL);

#endif /* IS_ENABLED(CONFIG_SETTINGS) */

static int activity_init(void) {

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&activity_save_work, activity_save_timeouts_work);
#endif

    activity_last_uptime = k_uptime_get();

    k_timer_start(&activity_timer, K_SECONDS(1), K_SECONDS(1));
    return 0;
}

ZMK_LISTENER(activity, activity_event_listener);
ZMK_SUBSCRIPTION(activity, zmk_position_state_changed);
ZMK_SUBSCRIPTION(activity, zmk_sensor_event);

#if IS_ENABLED(CONFIG_ZMK_POINTING)

static void note_activity_work_cb(struct k_work *_work) { note_activity(); }

K_WORK_DEFINE(note_activity_work, note_activity_work_cb);

static void activity_input_listener(struct input_event *ev) { k_work_submit(&note_activity_work); }

INPUT_CALLBACK_DEFINE(NULL, activity_input_listener);

#endif

SYS_INIT(activity_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
