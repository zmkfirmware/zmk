
/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/mouse.h>

struct zmk_mouse_button_state_changed {
    zmk_mouse_button_t buttons;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_mouse_button_state_changed);

static inline struct zmk_mouse_button_state_changed_event *
zmk_mouse_button_state_changed_from_encoded(uint32_t encoded, bool pressed, int64_t timestamp) {
    return new_zmk_mouse_button_state_changed((struct zmk_mouse_button_state_changed){
        .buttons = HID_USAGE_ID(encoded), .state = pressed, .timestamp = timestamp});
}
