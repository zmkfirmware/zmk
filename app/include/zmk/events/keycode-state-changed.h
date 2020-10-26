/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <dt-bindings/zmk/modifiers.h>
#include <zmk/event-manager.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

struct keycode_state_changed {
    struct zmk_event_header header;
    u8_t usage_page;
    u32_t keycode;
    u8_t implicit_modifiers;
    bool state;
};

ZMK_EVENT_DECLARE(keycode_state_changed);

static inline struct keycode_state_changed *
create_keycode_state_changed(u8_t usage_page, u32_t keycode, bool state) {
    struct keycode_state_changed *ev = new_keycode_state_changed();
    ev->usage_page = usage_page;
    ev->keycode = STRIP_MODS(keycode);
    ev->implicit_modifiers = SELECT_MODS(keycode);
    ev->state = state;
    return ev;
}

static inline struct keycode_state_changed *keycode_state_changed_from_encoded(u32_t encoded,
                                                                               bool pressed) {
    u16_t page = (encoded >> 16) & 0xFF;
    u16_t id = encoded & 0xFFFF;

    if (!page) {
        page = HID_USAGE_KEY;
    }

    return create_keycode_state_changed(page, id, pressed);
}