/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/hid.h>
#include <zmk/event_manager.h>

struct zmk_programmable_button_state_changed {
    uint8_t index;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_programmable_button_state_changed);

static inline int raise_zmk_programmable_button_state_changed_from_encoded(uint8_t index, bool pressed,
                                                                    int64_t timestamp) {
    return raise_zmk_programmable_button_state_changed((struct zmk_programmable_button_state_changed){
        .index = index, .state = pressed, .timestamp = timestamp});
}
