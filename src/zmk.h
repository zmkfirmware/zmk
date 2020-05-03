#ifndef ZMK_H
#define ZMK_H

#define __ZMK_MATRIX_NODE_ID DT_PATH(kscan)
#define ZMK_MATRIX_ROWS DT_PROP_LEN(__ZMK_MATRIX_NODE_ID,row_gpios)
#define ZMK_MATRIX_COLS DT_PROP_LEN(__ZMK_MATRIX_NODE_ID,col_gpios)

#endif
