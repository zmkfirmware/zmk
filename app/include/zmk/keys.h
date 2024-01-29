/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <dt-bindings/zmk/keys.h>

typedef uint32_t zmk_key_t;
typedef uint8_t zmk_mod_t;
typedef uint8_t zmk_mod_flags_t;

struct zmk_key_event {
    uint32_t column;
    uint32_t row;
    zmk_key_t key;
    bool pressed;
};

/**
 * Key data from a devicetree key code parameter.
 */
struct zmk_key_param {
    zmk_mod_flags_t modifiers;
    uint8_t page;
    uint16_t id;
};

/**
 * Decode a uint32_t devicetree key code parameter to a struct zmk_key_param.
 */
#define ZMK_KEY_PARAM_DECODE(param)                                                                \
    (struct zmk_key_param) {                                                                       \
        .modifiers = SELECT_MODS(param), .page = ZMK_HID_USAGE_PAGE(param),                        \
        .id = ZMK_HID_USAGE_ID(param),                                                             \
    }

static inline bool is_mod(uint8_t usage_page, uint32_t keycode) {
    return (keycode >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL &&
            keycode <= HID_USAGE_KEY_KEYBOARD_RIGHT_GUI && usage_page == HID_USAGE_KEY);
}