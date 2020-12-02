/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <dt-bindings/zmk/modifiers.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <zmk/event-manager.h>
#include <zmk/keys.h>

struct keycode_state_changed {
    struct zmk_event_header header;
    uint8_t usage_page;
    uint32_t keycode;
    uint8_t implicit_modifiers;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(keycode_state_changed);

static inline struct keycode_state_changed *
keycode_state_changed_from_encoded(uint32_t encoded, bool pressed, int64_t timestamp) {
    uint16_t page = HID_USAGE_PAGE(encoded) & 0xFF;
    uint16_t id = HID_USAGE_ID(encoded);
    zmk_mod_flags implicit_mods = SELECT_MODS(encoded);

    if (!page) {
        page = HID_USAGE_KEY;
    }

    struct keycode_state_changed *ev = new_keycode_state_changed();
    ev->usage_page = page;
    ev->keycode = id;
    ev->implicit_modifiers = implicit_mods;
    ev->state = pressed;
    ev->timestamp = timestamp;
    return ev;
}
