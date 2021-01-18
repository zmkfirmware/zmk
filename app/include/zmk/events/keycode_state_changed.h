/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <dt-bindings/zmk/modifiers.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <zmk/event_manager.h>
#include <zmk/keys.h>

struct zmk_keycode_state_changed {
    uint16_t usage_page;
    uint32_t keycode;
    uint8_t implicit_modifiers;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_keycode_state_changed);

static inline struct zmk_keycode_state_changed_event *
zmk_keycode_state_changed_from_encoded(uint32_t encoded, bool pressed, int64_t timestamp) {
    uint16_t page = HID_USAGE_PAGE(encoded) & 0xFF;
    uint16_t id = HID_USAGE_ID(encoded);
    zmk_mod_flags_t implicit_mods = SELECT_MODS(encoded);

    if (!page) {
        page = HID_USAGE_KEY;
    }

    return new_zmk_keycode_state_changed(
        (struct zmk_keycode_state_changed){.usage_page = page,
                                           .keycode = id,
                                           .implicit_modifiers = implicit_mods,
                                           .state = pressed,
                                           .timestamp = timestamp});
}
