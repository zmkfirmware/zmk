/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/mouse_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <zmk/endpoints.h>

static int hid_listener_keycode_pressed(const struct zmk_keycode_state_changed *ev) {
    int err;
    LOG_DBG("usage_page 0x%02X keycode 0x%02X implicit_mods 0x%02X explicit_mods 0x%02X",
            ev->usage_page, ev->keycode, ev->implicit_modifiers, ev->explicit_modifiers);
    switch (ev->usage_page) {
    case HID_USAGE_KEY:
        err = zmk_hid_keyboard_press(ev->keycode);
        if (err) {
            LOG_ERR("Unable to press keycode");
            return err;
        }
        break;
    case HID_USAGE_CONSUMER:
        err = zmk_hid_consumer_press(ev->keycode);
        if (err) {
            LOG_ERR("Unable to press keycode");
            return err;
        }
        break;
    case HID_USAGE_GD:
        err = zmk_hid_mouse_buttons_press(ev->keycode);
        if (err) {
            LOG_ERR("Unable to press button");
            return err;
        }
        break;
    }
    zmk_hid_register_mods(ev->explicit_modifiers);
    zmk_hid_implicit_modifiers_press(ev->implicit_modifiers);
    return zmk_endpoints_send_report(ev->usage_page);
}

static int hid_listener_keycode_released(const struct zmk_keycode_state_changed *ev) {
    int err;
    LOG_DBG("usage_page 0x%02X keycode 0x%02X implicit_mods 0x%02X explicit_mods 0x%02X",
            ev->usage_page, ev->keycode, ev->implicit_modifiers, ev->explicit_modifiers);
    switch (ev->usage_page) {
    case HID_USAGE_KEY:
        err = zmk_hid_keyboard_release(ev->keycode);
        if (err) {
            LOG_ERR("Unable to release keycode");
            return err;
        }
        break;
    case HID_USAGE_CONSUMER:
        err = zmk_hid_consumer_release(ev->keycode);
        if (err) {
            LOG_ERR("Unable to release keycode");
            return err;
        }
        break;
    case HID_USAGE_GD:
        err = zmk_hid_mouse_buttons_release(ev->keycode);
        if (err) {
            LOG_ERR("Unable to release button");
            return err;
        }
        break;
    }
    zmk_hid_unregister_mods(ev->explicit_modifiers);
    // There is a minor issue with this code.
    // If LC(A) is pressed, then LS(B), then LC(A) is released, the shift for B will be released
    // prematurely. This causes if LS(B) to repeat like Bbbbbbbb when pressed for a long time.
    // Solving this would require keeping track of which key's implicit modifiers are currently
    // active and only releasing modifiers at that time.
    zmk_hid_implicit_modifiers_release();
    return zmk_endpoints_send_report(ev->usage_page);
}

static int mouse_is_moving_counter = 0;

void mouse_timer_cb(struct k_timer *dummy);

K_TIMER_DEFINE(mouse_timer, mouse_timer_cb, NULL);

void mouse_timer_cb(struct k_timer *dummy)
{
    if (mouse_is_moving_counter != 0) {
        zmk_endpoints_send_mouse_report();
        k_timer_start(&mouse_timer, K_MSEC(10), K_NO_WAIT);
    }
}

static int hid_listener_mouse_pressed(const struct zmk_mouse_state_changed *ev) {
    int err;
    LOG_DBG("x: 0x%02X, y: 0x%02X", ev->x, ev->y);
    err = zmk_hid_mouse_movement_press(ev->x, ev->y);
    if (err) {
        LOG_ERR("Unable to press button");
        return err;
    }
    // race condition?
    mouse_is_moving_counter += 1;
    k_timer_start(&mouse_timer, K_MSEC(10), K_NO_WAIT);
    return zmk_endpoints_send_mouse_report();
}

static int hid_listener_mouse_released(const struct zmk_mouse_state_changed *ev) {
    int err;
    LOG_DBG("x: 0x%02X, y: 0x%02X", ev->x, ev->y);
    err = zmk_hid_mouse_movement_release(ev->x, ev->y);
    if (err) {
        LOG_ERR("Unable to release button");
        return err;
    }
    // race condition?
    mouse_is_moving_counter -= 1;
    return zmk_endpoints_send_mouse_report();
}

int hid_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev) {
        if (ev->state) {
            hid_listener_keycode_pressed(ev);
        } else {
            hid_listener_keycode_released(ev);
        }
    } else {
      const struct zmk_mouse_state_changed *ev = as_zmk_mouse_state_changed(eh);
      if (ev) {
        if (ev->state) {
          hid_listener_mouse_pressed(ev);
        } else {
          hid_listener_mouse_released(ev);
        }
      }
    }
    return 0;
}

ZMK_LISTENER(hid_listener, hid_listener);
ZMK_SUBSCRIPTION(hid_listener, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(hid_listener, zmk_mouse_state_changed);
