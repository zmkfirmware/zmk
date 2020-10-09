/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event-manager.h>

struct keycode_state_changed {
    struct zmk_event_header header;
    u8_t usage_page;
    u32_t keycode;
    bool state;
};

ZMK_EVENT_DECLARE(keycode_state_changed);

inline struct keycode_state_changed *create_keycode_state_changed(u8_t usage_page, u32_t keycode,
                                                                  bool state) {
    struct keycode_state_changed *ev = new_keycode_state_changed();
    ev->usage_page = usage_page;
    ev->keycode = keycode;
    ev->state = state;
    return ev;
}