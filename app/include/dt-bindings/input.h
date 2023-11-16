/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#define ZMK_INPUT_EXPLICIT_CODE(type, code) ((type << 16) & code)

#define ZMK_INPUT_EXPLICIT_CODE_TYPE(val) ((val >> 16) & 0xFF)
#define ZMK_INPUT_EXPLICIT_CODE_CODE(val) (val & 0xFFFF)