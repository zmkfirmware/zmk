
#include <zephyr.h>
#include <zmk/matrix_transform.h>
#include <zmk/matrix.h>
#include <dt-bindings/zmk/matrix-transform.h>

#define HAS_TRANSFORM DT_HAS_CHOSEN(zmk_matrix_transform)

#if HAS_TRANSFORM
#define ZMK_KEYMAP_TRANSFORM_NODE DT_CHOSEN(zmk_matrix_transform)
#define ZMK_KEYMAP_LEN DT_PROP_LEN(ZMK_KEYMAP_TRANSFORM_NODE, map)

#define _TRANSFORM_ENTRY(i, _) \
	[(KT_ROW(DT_PROP_BY_IDX(ZMK_KEYMAP_TRANSFORM_NODE, map, i)) * ZMK_MATRIX_COLS) + KT_COL(DT_PROP_BY_IDX(ZMK_KEYMAP_TRANSFORM_NODE, map, i))] = i,

static u32_t transform[] = 
    { UTIL_LISTIFY(ZMK_KEYMAP_LEN, _TRANSFORM_ENTRY, 0) };

#endif

u32_t zmk_matrix_transform_row_column_to_position(u32_t row, u32_t column)
{
    u32_t matrix_index = (row * ZMK_MATRIX_COLS) + column;

#if HAS_TRANSFORM
    return transform[matrix_index];
#else
    return matrix_index;
#endif /* HAS_TRANSFORM */
};
