/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/keys.h>

struct zmk_keycode_state_changed {
    int64_t timestamp;
    zmk_key_t keycode;
    uint16_t usage_page;
    zmk_mod_flags_t implicit_modifiers;
    zmk_mod_flags_t explicit_modifiers;
    bool state;
};

ZMK_EVENT_DECLARE(zmk_keycode_state_changed);

static inline struct zmk_keycode_state_changed
zmk_keycode_state_changed_from_encoded(uint32_t encoded, bool pressed, int64_t timestamp) {
    struct zmk_key_param key = ZMK_KEY_PARAM_DECODE(encoded);
    zmk_mod_flags_t implicit_modifiers = 0x00;
    zmk_mod_flags_t explicit_modifiers = 0x00;

    if (!key.page) {
        key.page = HID_USAGE_KEY;
    }

    if (is_mod(key.page, key.id)) {
        explicit_modifiers = key.modifiers;
    } else {
        implicit_modifiers = key.modifiers;
    }

    return (struct zmk_keycode_state_changed){.usage_page = key.page,
                                              .keycode = key.id,
                                              .implicit_modifiers = implicit_modifiers,
                                              .explicit_modifiers = explicit_modifiers,
                                              .state = pressed,
                                              .timestamp = timestamp};
}

static inline int raise_zmk_keycode_state_changed_from_encoded(uint32_t encoded, bool pressed,
                                                               int64_t timestamp) {
    return raise_zmk_keycode_state_changed(
        zmk_keycode_state_changed_from_encoded(encoded, pressed, timestamp));
}
