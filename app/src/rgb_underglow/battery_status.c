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

#include <zephyr/logging/log.h>

#include <zmk/rgb_underglow/init.h>
#include <zmk/rgb_underglow/rgb_underglow_base.h>
#include <zmk/rgb_underglow/startup_mutex.h>
#include <zmk/rgb_underglow/current_status.h>
#include <zmk/rgb_underglow/battery_status.h>

#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
uint8_t last_state_of_charge = 100;

int rgb_underglow_set_color_battery(uint8_t state_of_charge) {
    last_state_of_charge = state_of_charge;
    if (state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_CRIT) {
        struct zmk_led_hsb color = {h : 0, s : 100, b : 5};
        return zmk_rgb_ug_on() | zmk_rgb_ug_set_spd(5) |
               zmk_rgb_ug_select_effect(UNDERGLOW_EFFECT_BREATHE) | zmk_rgb_ug_set_hsb(color);
    } else if (state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_LOW) {
        struct zmk_led_hsb color = {h : 60, s : 100, b : 30};
        return zmk_rgb_ug_on() | zmk_rgb_ug_select_effect(UNDERGLOW_EFFECT_SOLID) |
               zmk_rgb_ug_set_hsb(color);
    } else {
        struct zmk_led_hsb color = {h : 120, s : 100, b : 30};
        return zmk_rgb_ug_on() | zmk_rgb_ug_select_effect(UNDERGLOW_EFFECT_SOLID) |
               zmk_rgb_ug_set_hsb(color);
    }
}

static void rgb_underglow_status_timeout_work(struct k_work *work) {
    zmk_rgb_underglow_apply_current_state();
}

K_WORK_DEFINE(underglow_timeout_work, rgb_underglow_status_timeout_work);

static void rgb_underglow_status_timeout_timer(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_timeout_work);
}

K_TIMER_DEFINE(underglow_timeout_timer, rgb_underglow_status_timeout_timer, NULL);

static int rgb_underglow_battery_state_event_listener(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *sc = as_zmk_battery_state_changed(eh);
    if (!sc) {
        LOG_ERR("underglow battery state listener called with unsupported argument");
        return -ENOTSUP;
    }

    if (is_starting_up())
        return 0;

    if (sc->state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_CRIT) {
        struct zmk_led_hsb color = {h : 0, s : 100, b : 5};
        last_state_of_charge = sc->state_of_charge;
        return zmk_rgb_ug_on() | zmk_rgb_ug_set_spd(5) |
               zmk_rgb_ug_select_effect(UNDERGLOW_EFFECT_BREATHE) | zmk_rgb_ug_set_hsb(color);
    }

    if (sc->state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_LOW &&
        (last_state_of_charge >= CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_LOW ||
         last_state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_CRIT)) {
        k_timer_start(&underglow_timeout_timer, K_SECONDS(5), K_NO_WAIT);
        last_state_of_charge = sc->state_of_charge;
        return rgb_underglow_set_color_battery(sc->state_of_charge);
    }

    if (sc->state_of_charge >= CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_LOW &&
        last_state_of_charge < CONFIG_ZMK_RGB_UNDERGLOW_BATTERY_LOW) {
        k_timer_start(&underglow_timeout_timer, K_SECONDS(5), K_NO_WAIT);
        last_state_of_charge = sc->state_of_charge;
        return rgb_underglow_set_color_battery(sc->state_of_charge);
    }

    last_state_of_charge = sc->state_of_charge;
    return 0;
}

ZMK_LISTENER(rgb_battery, rgb_underglow_battery_state_event_listener);
ZMK_SUBSCRIPTION(rgb_battery, zmk_battery_state_changed);
