/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <dt-bindings/zmk/midi.h>

typedef uint8_t zmk_midi_cin_t;
typedef uint16_t zmk_midi_key_t;
typedef uint8_t zmk_midi_value_t;

// used for bitmaps
typedef uint64_t zmk_midi_keys_t;
