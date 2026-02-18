/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <math.h>
#include <stdlib.h>

#include <zmk/rgb_underglow/init.h>
#include <zmk/rgb_underglow/rgb_underglow_base.h>
#include <zmk/rgb_underglow/startup_mutex.h>
#include <zmk/rgb_underglow/current_status.h>
#include <zmk/rgb_underglow/ble_peripheral_status.h>

#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/workqueue.h>

struct peripheral_ble_state zmk_get_ble_peripheral_state() {
    return (struct peripheral_ble_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

int zmk_rgb_underglow_set_color_ble_peripheral(struct peripheral_ble_state ps) {
    struct zmk_led_hsb color = {h : 240, s : 100, b : 30};
    if (ps.connected)
        return zmk_rgb_ug_on() | zmk_rgb_ug_select_effect(UNDERGLOW_EFFECT_SOLID) |
               zmk_rgb_ug_set_hsb(color);
    return zmk_rgb_ug_on() | zmk_rgb_ug_set_spd(5) |
           zmk_rgb_ug_select_effect(UNDERGLOW_EFFECT_BREATHE) | zmk_rgb_ug_set_hsb(color);
}

static void rgb_underglow_ble_peripheral_status_timeout_work(struct k_work *work) {
    zmk_rgb_underglow_apply_current_state();
}

K_WORK_DEFINE(underglow_ble_peripheral_timeout_work,
              rgb_underglow_ble_peripheral_status_timeout_work);

static void rgb_underglow_ble_peripheral_status_timeout_timer(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_ble_peripheral_timeout_work);
}

K_TIMER_DEFINE(underglow_ble_peripheral_timeout_timer,
               rgb_underglow_ble_peripheral_status_timeout_timer, NULL);

static int rgb_underglow_ble_peripheral_state_event_listener(const zmk_event_t *eh) {
    const struct peripheral_ble_state state = zmk_get_ble_peripheral_state();

    if (is_starting_up())
        return 0;

    if (state.connected)
        k_timer_start(&underglow_ble_peripheral_timeout_timer, K_SECONDS(2), K_NO_WAIT);

    return zmk_rgb_underglow_set_color_ble_peripheral(state);
}

ZMK_LISTENER(rgb_ble_peripheral, rgb_underglow_ble_peripheral_state_event_listener);
ZMK_SUBSCRIPTION(rgb_ble_peripheral, zmk_split_peripheral_status_changed);
