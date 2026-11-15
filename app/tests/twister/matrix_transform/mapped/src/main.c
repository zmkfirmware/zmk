/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>

#include <zmk/matrix_transform.h>

ZMK_MATRIX_TRANSFORM_EXTERN(DT_NODELABEL(offset_transform));
ZMK_MATRIX_TRANSFORM_EXTERN(DT_NODELABEL(wide_transform));
ZMK_MATRIX_TRANSFORM_EXTERN(DT_NODELABEL(single_row_transform));

ZTEST(matrix_transform_mapped, test_offset_transform_maps_scanned_positions) {
    zmk_matrix_transform_t mt = ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_NODELABEL(offset_transform));

    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 0), 0);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 1), 1);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 1, 1), 2);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 1, 2), 3);
}

ZTEST(matrix_transform_mapped, test_offset_transform_unmapped_positions_are_invalid) {
    zmk_matrix_transform_t mt = ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_NODELABEL(offset_transform));

    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 2), -EINVAL);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 2, 0), -EINVAL);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 3, 3), -EINVAL);
}

ZTEST(matrix_transform_mapped, test_wide_transform_maps_scanned_positions) {
    zmk_matrix_transform_t mt = ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_NODELABEL(wide_transform));

    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 0), 0);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 4), 1);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 1, 0), 2);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 1, 4), 3);
}

ZTEST(matrix_transform_mapped, test_wide_transform_unmapped_positions_are_invalid) {
    zmk_matrix_transform_t mt = ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_NODELABEL(wide_transform));

    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 1), -EINVAL);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 1, 5), -EINVAL);
}

ZTEST(matrix_transform_mapped, test_single_row_transform_maps_scanned_positions) {
    zmk_matrix_transform_t mt = ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_NODELABEL(single_row_transform));

    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 0), 0);
    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 0, 1), 1);
}

ZTEST(matrix_transform_mapped, test_single_row_transform_second_row_is_invalid) {
    zmk_matrix_transform_t mt = ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_NODELABEL(single_row_transform));

    zassert_equal(zmk_matrix_transform_row_column_to_position(mt, 1, 0), -EINVAL);
}

ZTEST_SUITE(matrix_transform_mapped, NULL, NULL, NULL, NULL, NULL);
