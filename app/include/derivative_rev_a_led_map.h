/*
 * Derivative Rev A LED Map Configuration
 *
 * 51 LEDs total:
 *   0-2:   Indicators (caps lock, BT status, layer status)
 *   3-50:  Per-key (48 keys)
 *
 * Layout: 6 + 12 + 11 + 11 + 8 keys
 */

#pragma once

#define UNDERGLOW_COUNT 0
static const uint8_t underglow_map[UNDERGLOW_COUNT] = {};

#define PER_KEY_COUNT 48
static const uint8_t per_key_map[PER_KEY_COUNT] = {
    3,  4,  5,  6,  7,  8,                         /* Row 0: 6 keys  */
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, /* Row 1: 12 keys */
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,    /* Row 2: 11 keys */
    42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,    /* Row 3: 11 keys */
    43, 44, 45, 46, 47, 48, 49, 50                 /* Row 4: 8 keys  */
};

#define PER_KEY_ROW_COUNT 5
static const uint8_t per_key_row_sizes[PER_KEY_ROW_COUNT] = {6, 12, 11, 11, 8};

#define PER_KEY_COL_MAX 12
static const uint8_t per_key_col[PER_KEY_COUNT] = {
    0, 1, 2, 3, 4, 5,                     /* Row 0: 6 keys  */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /* Row 1: 12 keys */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,     /* Row 2: 11 keys */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,     /* Row 3: 11 keys */
    0, 1, 2, 3, 4, 5, 6, 7,               /* Row 4: 8 keys  */
};
