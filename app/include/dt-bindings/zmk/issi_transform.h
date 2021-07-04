/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * Maps a 39x9 matrix cell index to the appropriate IS31FL3741 register indexes.
 */
#define PIXEL(n) (                                  \
	((0 <= ((n) % 39)) && (((n) % 39) < 30))            \
		? ((((n) / 39) * 30) + ((n) % 39))              \
		: (210 + (((n) / 39) * 9) + (((n) % 39) - 31)))

#define RGB(com, r, g, b) PIXEL(com + r) PIXEL(com + g) PIXEL(com + b)

#define SW(n) ((n - 1) * 39)
#define CS(n) (n - 1)
