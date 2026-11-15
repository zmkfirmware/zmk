/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>

#include <zmk/matrix_transform.h>

ZMK_MATRIX_TRANSFORM_DEFAULT_EXTERN();

ZTEST(matrix_transform_fallback, test_positions_use_row_column_arithmetic) {
    zmk_matrix_transform_t mt = &zmk_matrix_transform_default;

    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 0), 0);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 2), 2);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 1, 0), 3);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 1, 2), 5);
}

ZTEST_SUITE(matrix_transform_fallback, NULL, NULL, NULL, NULL, NULL);
