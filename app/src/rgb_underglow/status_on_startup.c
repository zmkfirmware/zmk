/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define IS_PERIPHERAL (IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <math.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zmk/rgb_underglow/rgb_underglow_base.h>
#include <zmk/rgb_underglow/startup_mutex.h>
#include <zmk/rgb_underglow/current_status.h>
#include <zmk/rgb_underglow/battery_status.h>
#include <zmk/rgb_underglow/ble_status.h>
#include <zmk/rgb_underglow/ble_peripheral_status.h>

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>

#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum STARTUP_STATE {
    BATTERY,
    CONNECTING,
    CONNECTED,
};
static enum zmk_activity_state last_activity_state = ZMK_ACTIVITY_SLEEP;
static int64_t last_checkpoint = 0;
#if CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS
static enum STARTUP_STATE startup_state = BATTERY;
#else
static enum STARTUP_STATE startup_state = CONNECTING;
#endif // CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS
static bool switched_startup_state = true;
static struct k_timer *running_timer;

static void zmk_on_startup_timer_tick_work(struct k_work *work) {
#if CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS
    uint8_t state_of_charge = zmk_battery_state_of_charge();
#endif // CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS
#if CONFIG_ZMK_RGB_UNDERGLOW_BLE_STATUS
#if !IS_PERIPHERAL
    struct output_state os = zmk_get_output_state();
    if (os.selected_endpoint.transport == ZMK_TRANSPORT_USB) {
        k_timer_stop(running_timer);
        zmk_rgb_underglow_apply_current_state();
        return;
    }
#else
    struct peripheral_ble_state ps = zmk_get_ble_peripheral_state();
#endif // !IS_PERIPHERAL
#endif // CONFIG_ZMK_RGB_UNDERGLOW_BLE_STATUS

    int64_t uptime = k_uptime_get();
    if (last_checkpoint + 3000 < uptime && startup_state != CONNECTING) {
        switch (startup_state) {
        case BATTERY:
#if CONFIG_ZMK_RGB_UNDERGLOW_BLE_STATUS
#if !IS_PERIPHERAL
            startup_state = os.active_profile_connected ? CONNECTED : CONNECTING;
#else
            startup_state = ps.connected ? CONNECTED : CONNECTING;
#endif // !IS_PERIPHERAL
            switched_startup_state = true;
#else
            k_timer_stop(running_timer); // probably won't work
            zmk_rgb_underglow_apply_current_state();
            return;
#endif // CONFIG_ZMK_RGB_UNDERGLOW_BLE_STATUS
            last_checkpoint = uptime;
            break;
        case CONNECTED:
            k_timer_stop(running_timer); // probably won't work
            zmk_rgb_underglow_apply_current_state();
            return;
        }
    }

#if CONFIG_ZMK_RGB_UNDERGLOW_BLE_STATUS
#if !IS_PERIPHERAL
    if (startup_state == CONNECTING && os.active_profile_connected) {
#else
    if (startup_state == CONNECTING && ps.connected) {
#endif // !IS_PERIPHERAL
        startup_state = CONNECTED;
        last_checkpoint = uptime;
        switched_startup_state = true;
    }
#endif // CONFIG_ZMK_RGB_UNDERGLOW_BLE_STATUS

    if (switched_startup_state) {
        switched_startup_state = false;
        switch (startup_state) {
#if CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS
        case BATTERY:
            rgb_underglow_set_color_battery(state_of_charge);
            return;
#endif // CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS
#if CONFIG_ZMK_RGB_UNDERGLOW_BLE_STATUS
        case CONNECTING:
#if !IS_PERIPHERAL
            zmk_rgb_underglow_set_color_ble(os);
#else
            zmk_rgb_underglow_set_color_ble_peripheral(ps);
#endif // !IS_PERIPHERAL

        case CONNECTED:
#if !IS_PERIPHERAL
            zmk_rgb_underglow_set_color_ble(os);
#else
            zmk_rgb_underglow_set_color_ble_peripheral(ps);
#endif // !IS_PERIPHERAL
#endif // CONFIG_ZMK_RGB_UNDERGLOW_BLE_STATUS
            return;
        default:
            return;
        }
    }
}

K_WORK_DEFINE(on_startup_timer_tick_work, zmk_on_startup_timer_tick_work);

static void on_startup_timer_tick_stop_cb(struct k_timer *timer) { stop_startup(); }

static void on_startup_timer_tick_cb(struct k_timer *timer) {
    running_timer = timer;
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &on_startup_timer_tick_work);
}

K_TIMER_DEFINE(on_startup_timer_tick, on_startup_timer_tick_cb, on_startup_timer_tick_stop_cb);

void init() {
    if (!start_startup()) {
        LOG_ERR("Cannot start startup sequence, startup sequence already started");
        return;
    }

#if CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS
    startup_state = BATTERY;
#else
    startup_state = CONNECTING;
#endif // CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_STATUS

    last_checkpoint = k_uptime_get();
    k_timer_start(&on_startup_timer_tick, K_NO_WAIT, K_MSEC(100));
}

int startup(const enum zmk_activity_state state) {
    switch (state) {
    case ZMK_ACTIVITY_ACTIVE:
        if (last_activity_state == ZMK_ACTIVITY_SLEEP) {
            init();
            break;
        }
    default:
        last_activity_state = state;
        if (is_starting_up()) {
            k_timer_stop(&on_startup_timer_tick);
            return zmk_rgb_underglow_apply_current_state();
        }
        break;
    }

    return 0;
}

int startup_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    return startup(ev->state);
}

static int startup_init(void) {
    last_activity_state = ZMK_ACTIVITY_SLEEP;
    return startup(ZMK_ACTIVITY_ACTIVE);
}

ZMK_LISTENER(status_on_startup, startup_handler);
ZMK_SUBSCRIPTION(status_on_startup, zmk_activity_state_changed);
SYS_INIT(startup_init, APPLICATION, CONFIG_ZMK_USB_HID_INIT_PRIORITY);
