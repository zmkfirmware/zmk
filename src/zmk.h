#ifndef ZMK_H
#define ZMK_H

#define ZMK_MATRIX_NODE_ID DT_CHOSEN(zmk_kscan)
#define ZMK_MATRIX_ROWS DT_PROP_LEN(ZMK_MATRIX_NODE_ID,row_gpios)
#define ZMK_MATRIX_COLS DT_PROP_LEN(ZMK_MATRIX_NODE_ID,col_gpios)

#endif
