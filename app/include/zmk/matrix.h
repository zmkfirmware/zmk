/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <devicetree.h>

#define ZMK_MATRIX_NODE_ID DT_CHOSEN(zmk_kscan)

#if DT_HAS_CHOSEN(zmk_matrix_transform)

#define ZMK_KEYMAP_TRANSFORM_NODE DT_CHOSEN(zmk_matrix_transform)
#define ZMK_KEYMAP_LEN DT_PROP_LEN(ZMK_KEYMAP_TRANSFORM_NODE, map)

#define ZMK_MATRIX_ROWS DT_PROP(ZMK_KEYMAP_TRANSFORM_NODE, rows)
#define ZMK_MATRIX_COLS DT_PROP(ZMK_KEYMAP_TRANSFORM_NODE, columns)

#else /* DT_HAS_CHOSEN(zmk_matrix_transform) */

#if DT_NODE_HAS_PROP(ZMK_MATRIX_NODE_ID, row_gpios)
#define ZMK_MATRIX_ROWS DT_PROP_LEN(ZMK_MATRIX_NODE_ID, row_gpios)
#define ZMK_MATRIX_COLS DT_PROP_LEN(ZMK_MATRIX_NODE_ID, col_gpios)
#elif DT_NODE_HAS_PROP(ZMK_MATRIX_NODE_ID, input_gpios)
#define ZMK_MATRIX_ROWS 1
#define ZMK_MATRIX_COLS DT_PROP_LEN(ZMK_MATRIX_NODE_ID, input_gpios)
#else
#define ZMK_MATRIX_ROWS DT_PROP(ZMK_MATRIX_NODE_ID, rows)
#define ZMK_MATRIX_COLS DT_PROP(ZMK_MATRIX_NODE_ID, columns)
#endif

#define ZMK_KEYMAP_LEN (ZMK_MATRIX_COLS * ZMK_MATRIX_ROWS)

#endif /* !DT_HAS_CHOSEN(zmk_matrix_transform) */