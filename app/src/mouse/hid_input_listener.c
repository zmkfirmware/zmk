/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zmk/mouse.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>

void handle_rel_code(struct input_event *evt) {
    switch (evt->code) {
    case INPUT_REL_X:
        zmk_hid_mouse_movement_update(evt->value, 0);
        break;
    case INPUT_REL_Y:
        zmk_hid_mouse_movement_update(0, evt->value);
        break;
    case INPUT_REL_WHEEL:
        zmk_hid_mouse_scroll_update(0, evt->value);
        break;
    case INPUT_REL_HWHEEL:
        zmk_hid_mouse_scroll_update(evt->value, 0);
        break;
    default:
        break;
    }
}

void handle_key_code(struct input_event *evt) {
    int8_t btn;

    switch (evt->code) {
    case INPUT_BTN_0:
    case INPUT_BTN_1:
    case INPUT_BTN_2:
    case INPUT_BTN_3:
    case INPUT_BTN_4:
        btn = evt->code - INPUT_BTN_0;
        if (evt->value > 0) {
            zmk_hid_mouse_button_press(btn);
        } else {
            zmk_hid_mouse_button_release(btn);
        }
        break;
    default:
        break;
    }
}

void input_handler(struct input_event *evt) {
    switch (evt->type) {
    case INPUT_EV_REL:
        handle_rel_code(evt);
        break;
    case INPUT_EV_KEY:
        handle_key_code(evt);
        break;
    }

    if (evt->sync) {
        zmk_endpoints_send_mouse_report();
        zmk_hid_mouse_scroll_set(0, 0);
        zmk_hid_mouse_movement_set(0, 0);
    }
}

INPUT_CALLBACK_DEFINE(NULL, input_handler);
