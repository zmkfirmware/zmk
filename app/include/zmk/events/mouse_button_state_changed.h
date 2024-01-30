
/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/mouse.h>

struct zmk_mouse_button_state_changed {
    zmk_mouse_button_t buttons;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_mouse_button_state_changed);

static inline int raise_zmk_mouse_button_state_changed_from_encoded(uint32_t encoded, bool pressed,
                                                                    int64_t timestamp) {
    return raise_zmk_mouse_button_state_changed((struct zmk_mouse_button_state_changed){
        .buttons = ZMK_HID_USAGE_ID(encoded), .state = pressed, .timestamp = timestamp});
}
