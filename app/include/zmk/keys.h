/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <dt-bindings/zmk/keys.h>

typedef uint32_t zmk_key_t;
typedef uint8_t zmk_mod_t;
typedef uint8_t zmk_mod_flags;

struct zmk_key_event {
    uint32_t column;
    uint32_t row;
    zmk_key_t key;
    bool pressed;
};