/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <dt-bindings/zmk/mouse.h>

typedef uint8_t zmk_mouse_button_flags_t;
typedef uint16_t zmk_mouse_button_t;

int zmk_mouse_init(void);
