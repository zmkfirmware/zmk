/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <dt-bindings/zmk/keys.h>

typedef u32_t zmk_key;
typedef u8_t zmk_action;
typedef u8_t zmk_mod;
typedef u8_t zmk_mod_flags;

struct zmk_key_event {
    u32_t column;
    u32_t row;
    zmk_key key;
    bool pressed;
};