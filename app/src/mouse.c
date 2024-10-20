/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/events/mouse_button_state_changed.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/mouse.h>

static void listener_mouse_button_pressed(const struct zmk_mouse_button_state_changed *ev) {
    LOG_DBG("buttons: 0x%02X", ev->buttons);
    zmk_hid_mouse_buttons_press(ev->buttons);
    zmk_endpoints_send_mouse_report();
}

static void listener_mouse_button_released(const struct zmk_mouse_button_state_changed *ev) {
    LOG_DBG("buttons: 0x%02X", ev->buttons);
    zmk_hid_mouse_buttons_release(ev->buttons);
    zmk_endpoints_send_mouse_report();
}

int mouse_listener(const zmk_event_t *eh) {
    const struct zmk_mouse_button_state_changed *mbt_ev = as_zmk_mouse_button_state_changed(eh);
    if (mbt_ev) {
        if (mbt_ev->state) {
            listener_mouse_button_pressed(mbt_ev);
        } else {
            listener_mouse_button_released(mbt_ev);
        }
        return 0;
    }
    return 0;
}

ZMK_LISTENER(mouse_listener, mouse_listener);
ZMK_SUBSCRIPTION(mouse_listener, zmk_mouse_button_state_changed);
