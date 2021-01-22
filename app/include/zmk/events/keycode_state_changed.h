/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>
#include <zmk/keys.h>

struct zmk_keycode_state_changed {
    uint16_t usage_page;
    uint32_t keycode;
    uint8_t implicit_modifiers;
    uint8_t explicit_modifiers;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_keycode_state_changed);

static inline struct zmk_keycode_state_changed_event *
zmk_keycode_state_changed_from_encoded(uint32_t encoded, bool pressed, int64_t timestamp) {
    uint16_t page = HID_USAGE_PAGE(encoded) & 0xFF;
    uint16_t id = HID_USAGE_ID(encoded);
    uint8_t implicit_modifiers = 0x00;
    uint8_t explicit_modifiers = 0x00;

    if (!page) {
        page = HID_USAGE_KEY;
    }

    if (is_mod(page, id)) {
        explicit_modifiers = SELECT_MODS(encoded);
    } else {
        implicit_modifiers = SELECT_MODS(encoded);
    }

    return new_zmk_keycode_state_changed(
        (struct zmk_keycode_state_changed){.usage_page = page,
                                           .keycode = id,
                                           .implicit_modifiers = implicit_modifiers,
                                           .explicit_modifiers = explicit_modifiers,
                                           .state = pressed,
                                           .timestamp = timestamp});
}
