/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#define ZMK_MOCK_IS_PRESS(v) ((v & (0x01 << 31)) != 0)
#define ZMK_MOCK_PRESS(row, col, msec) (row + (col << 8) + (msec << 16) + (0x01 << 31))
#define ZMK_MOCK_RELEASE(row, col, msec) (row + (col << 8) + (msec << 16))
#define ZMK_MOCK_ROW(v) (v & 0xFF)
#define ZMK_MOCK_COL(v) ((v >> 8) & 0xFF)
#define ZMK_MOCK_MSEC(v) ((v & ~(0x01 << 31)) >> 16)
