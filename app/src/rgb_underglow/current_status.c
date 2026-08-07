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
#include <zmk/rgb_underglow/current_status.h>

#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_CHOSEN(zmk_underglow)

#error "A zmk,underglow chosen node must be declared"

#endif

BUILD_ASSERT(CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN <= CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX,
             "ERROR: RGB underglow maximum brightness is less than minimum brightness");

static struct rgb_underglow_state *preserved_state;

int zmk_rgb_underglow_get_state(bool *on_off) {
    *on_off = preserved_state->on;
    return 0;
}

int zmk_rgb_underglow_on(void) {
    preserved_state->on = true;
    return zmk_rgb_ug_on() | zmk_rgb_ug_save_state();
}

int zmk_rgb_underglow_off(void) {
    preserved_state->on = false;
    return zmk_rgb_ug_off() | zmk_rgb_ug_save_state();
}

int zmk_rgb_underglow_calc_effect(int direction) {
    return (preserved_state->current_effect + UNDERGLOW_EFFECT_NUMBER + direction) %
           UNDERGLOW_EFFECT_NUMBER;
}

int zmk_rgb_underglow_select_effect(int effect) {
    preserved_state->current_effect = effect;
    return zmk_rgb_ug_select_effect(effect) | zmk_rgb_ug_save_state();
}

int zmk_rgb_underglow_cycle_effect(int direction) {
    return zmk_rgb_underglow_select_effect(zmk_rgb_underglow_calc_effect(direction));
}

int zmk_rgb_underglow_toggle(void) {
    return preserved_state->on ? zmk_rgb_underglow_off()
                               : zmk_rgb_underglow_on() | zmk_rgb_ug_save_state();
}

int zmk_rgb_underglow_set_hsb(struct zmk_led_hsb color) {
    preserved_state->color = color;
    return zmk_rgb_ug_set_hsb(color) | zmk_rgb_ug_save_state();
}

struct zmk_led_hsb zmk_rgb_underglow_calc_hue(int direction) {
    struct zmk_led_hsb color = preserved_state->color;

    color.h += HUE_MAX + (direction * CONFIG_ZMK_RGB_UNDERGLOW_HUE_STEP);
    color.h %= HUE_MAX;

    return color;
}

struct zmk_led_hsb zmk_rgb_underglow_calc_sat(int direction) {
    struct zmk_led_hsb color = preserved_state->color;

    int s = color.s + (direction * CONFIG_ZMK_RGB_UNDERGLOW_SAT_STEP);
    if (s < 0) {
        s = 0;
    } else if (s > SAT_MAX) {
        s = SAT_MAX;
    }
    color.s = s;

    return color;
}

struct zmk_led_hsb zmk_rgb_underglow_calc_brt(int direction) {
    struct zmk_led_hsb color = preserved_state->color;

    int b = color.b + (direction * CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP);
    color.b = CLAMP(b, 0, BRT_MAX);

    return color;
}

int zmk_rgb_underglow_change_hue(int direction) {
    preserved_state->color = zmk_rgb_underglow_calc_hue(direction);
    return zmk_rgb_ug_set_hsb(preserved_state->color) | zmk_rgb_ug_save_state();
}

int zmk_rgb_underglow_change_sat(int direction) {
    preserved_state->color = zmk_rgb_underglow_calc_sat(direction);
    return zmk_rgb_ug_set_hsb(preserved_state->color) | zmk_rgb_ug_save_state();
}

int zmk_rgb_underglow_change_brt(int direction) {
    preserved_state->color = zmk_rgb_underglow_calc_brt(direction);
    return zmk_rgb_ug_set_hsb(preserved_state->color) | zmk_rgb_ug_save_state();
}

int zmk_rgb_underglow_change_spd(int direction) {
    int speed = preserved_state->animation_speed + direction;
    speed = CLAMP(speed, 1, 5);
    preserved_state->animation_speed = speed;
    return zmk_rgb_ug_set_spd(speed) | zmk_rgb_ug_save_state();
}

int zmk_rgb_underglow_apply_current_state(void) {

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE)
    if (zmk_activity_get_state() != ZMK_ACTIVITY_ACTIVE) {
        return zmk_rgb_ug_off();
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
    if (!zmk_usb_is_powered()) {
        return zmk_rgb_ug_off();
    }
#endif

    if (zmk_rgb_ug_set_hsb(preserved_state->color) ||
        zmk_rgb_ug_set_spd(preserved_state->animation_speed) ||
        zmk_rgb_ug_select_effect(preserved_state->current_effect)) {
        LOG_ERR("Failed to set the current rgb config");
        return 0;
    }

    return preserved_state->on ? zmk_rgb_ug_on() : zmk_rgb_ug_off();
}

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE) ||                                          \
    IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
static int rgb_underglow_auto_state(bool new_state) {
    if (new_state) {
        return zmk_rgb_underglow_on();
    } else {
        return zmk_rgb_underglow_off();
    }
}

static int rgb_underglow_event_listener(const zmk_event_t *eh) {

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE)
    if (as_zmk_activity_state_changed(eh)) {
        return rgb_underglow_auto_state(zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
    if (as_zmk_usb_conn_state_changed(eh)) {
        return rgb_underglow_auto_state(zmk_usb_is_powered());
    }
#endif

    return -ENOTSUP;
}

ZMK_LISTENER(rgb_underglow, rgb_underglow_event_listener);
#endif // IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE) ||
       // IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)

void zmk_rgb_underglow_init(void) { preserved_state = zmk_rgb_ug_get_save_state(); }

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE)
ZMK_SUBSCRIPTION(rgb_underglow, zmk_activity_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
ZMK_SUBSCRIPTION(rgb_underglow, zmk_usb_conn_state_changed);
#endif
