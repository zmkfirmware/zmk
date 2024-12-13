/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/sys/util.h>

typedef const struct zmk_matrix_transform *zmk_matrix_transform_t;

#define ZMK_MATRIX_TRANSFORM_DEFAULT_EXTERN()                                                      \
    extern const struct zmk_matrix_transform zmk_matrix_transform_default
#define ZMK_MATRIX_TRANSFORM_EXTERN(node_id)                                                       \
    extern const struct zmk_matrix_transform _CONCAT(zmk_matrix_transform_, node_id)

#define ZMK_MATRIX_TRANSFORM_T_FOR_NODE(node_id) &_CONCAT(zmk_matrix_transform_, node_id)

int32_t zmk_matrix_transform_row_column_to_position(zmk_matrix_transform_t mt, uint32_t row,
                                                    uint32_t column);
