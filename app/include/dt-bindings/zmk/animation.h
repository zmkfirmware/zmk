/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * Maps HSL color settings into a single uint32_t value
 * that can be cast to zmk_color_hsl.
 */
#ifdef CONFIG_BIG_ENDIAN
#define HSL(h, s, l) ((h << 16) + (s << 8) + l)
#else
#define HSL(h, s, l) (h + (s << 16) + (l << 24))
#endif
