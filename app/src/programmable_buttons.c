/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/events/programmable_button_state_changed.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>

static void listener_programmable_button_pressed(const struct zmk_programmable_button_state_changed *ev) {
    LOG_DBG("programmable button event press: 0x%02X", ev->index);
    zmk_hid_programmable_button_press(ev->index);
    zmk_endpoints_send_programmable_buttons_report();
}

static void listener_programmable_button_released(const struct zmk_programmable_button_state_changed *ev) {
    LOG_DBG("programmable button event released: 0x%02X", ev->index);
    zmk_hid_programmable_button_release(ev->index);
    zmk_endpoints_send_programmable_buttons_report();
}

int programmable_buttons_listener(const zmk_event_t *eh) {
    const struct zmk_programmable_button_state_changed *pb_ev = as_zmk_programmable_button_state_changed(eh);
    if (pb_ev) {
        if (pb_ev->state) {
            listener_programmable_button_pressed(pb_ev);
        } else {
            listener_programmable_button_released(pb_ev);
        }
        return 0;
    }
    return 0;
}

ZMK_LISTENER(programmable_buttons_listener, programmable_buttons_listener);
ZMK_SUBSCRIPTION(programmable_buttons_listener, zmk_programmable_button_state_changed);
