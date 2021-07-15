/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * Maps a 39x9 matrix cell index to a 1D array matching IS31FL3741's PWM registers order.
 */
#define PIXEL(n) (                                    \
    (((n) % 39) < 30)                                 \
        ? ((((n) / 39) * 30) + ((n) % 39))            \
        : (270 + (((n) / 39) * 9) + ((n) % 39) - 30))

#define RGB(com, r, g, b) PIXEL(com + r) PIXEL(com + g) PIXEL(com + b)

#define SW(n) ((n - 1) * 39)
#define CS(n) (n - 1)
