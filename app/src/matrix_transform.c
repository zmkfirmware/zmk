/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zmk/matrix_transform.h>
#include <zmk/matrix.h>
#include <dt-bindings/zmk/matrix_transform.h>

#define DT_DRV_COMPAT zmk_matrix_transform

struct zmk_matrix_transform {
    uint32_t const *lookup_table;
    size_t len;
    uint8_t rows;
    uint8_t columns;
    uint8_t col_offset;
    uint8_t row_offset;
};

/* the transform in the device tree is a list of (row,column) pairs that is
 * indexed by by the keymap position of that key. We want to invert this in
 * order to be able to quickly determine what keymap position a particular
 * row,column pair is associated with, using a single lookup.
 *
 * We do this by creating the `transform` array at compile time, which is
 * indexed by (row * ZMK_MATRIX_COLS) + column, and the value contains an
 * encoded keymap index it is associated with. The keymap index is encoded
 * by adding INDEX_OFFSET to it, because not all row,column pairs have an
 * associated keymap index (some matrices are sparse), C globals are
 * initialized to 0, and the keymap index of 0 is a valid index. We want to
 * be able to detect the condition when an unassigned matrix position is
 * pressed and we want to return an error.
 */

#define INDEX_OFFSET 1

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_matrix_transform)

#define TRANSFORM_LOOKUP_ENTRY(i, n)                                                               \
    [(KT_ROW(DT_INST_PROP_BY_IDX(n, map, i)) * DT_INST_PROP(n, columns)) +                         \
        KT_COL(DT_INST_PROP_BY_IDX(n, map, i))] = i + INDEX_OFFSET

#define MATRIX_TRANSFORM_INIT(n)                                                                   \
    static const uint32_t _CONCAT(zmk_transform_lookup_table_, n)[] = {                            \
        LISTIFY(DT_INST_PROP_LEN(n, map), TRANSFORM_LOOKUP_ENTRY, (, ), n)};                       \
    const struct zmk_matrix_transform _CONCAT(zmk_matrix_transform_, DT_DRV_INST(n)) = {           \
        .rows = DT_INST_PROP(n, rows),                                                             \
        .columns = DT_INST_PROP(n, columns),                                                       \
        .col_offset = DT_INST_PROP(n, col_offset),                                                 \
        .row_offset = DT_INST_PROP(n, row_offset),                                                 \
        .lookup_table = _CONCAT(zmk_transform_lookup_table_, n),                                   \
        .len = ARRAY_SIZE(_CONCAT(zmk_transform_lookup_table_, n)),                                \
    };

DT_INST_FOREACH_STATUS_OKAY(MATRIX_TRANSFORM_INIT);

#elif DT_HAS_CHOSEN(zmk_kscan) && defined(ZMK_MATRIX_COLS) && defined(ZMK_MATRIX_ROWS)

const struct zmk_matrix_transform zmk_matrix_transform_default = {
    .rows = ZMK_MATRIX_ROWS,
    .columns = ZMK_MATRIX_COLS,
    .len = ZMK_KEYMAP_LEN,
};

#else

#error "Need a matrix transform or compatible kscan selected to determine keymap size!"
`
#endif // DT_HAS_COMPAT_STATUS_OKAY(zmk_matrix_transform)

int32_t zmk_matrix_transform_row_column_to_position(zmk_matrix_transform_t mt, uint32_t row,
                                                    uint32_t column) {
    column += mt->col_offset;
    row += mt->row_offset;

    if (!mt->lookup_table) {
        return (row * mt->columns) + column;
    }

    uint16_t lookup_index = ((row * mt->columns) + column);
    if (lookup_index >= mt->len) {
        return -EINVAL;
    }

    int32_t val = mt->lookup_table[lookup_index];
    if (val == 0) {
        return -EINVAL;
    }

    return val - INDEX_OFFSET;
};