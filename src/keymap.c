
#include "keymap.h"

static enum hid_kbd_code zmk_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_MATRIX_ROWS * ZMK_MATRIX_COLS] = {
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE,layers,0)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE,layers,0,keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE,layers,1)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE,layers,1,keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE,layers,2)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE,layers,2,keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE,layers,3)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE,layers,3,keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE,layers,4)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE,layers,4,keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE,layers,5)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE,layers,5,keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE,layers,6)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE,layers,6,keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE,layers,7)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE,layers,7,keys),
#endif
};

enum hid_kbd_code zmk_keymap_keycode_from_position(u32_t row, u32_t column)
{
	return zmk_keymap[0][(row * ZMK_MATRIX_ROWS) + column];
}
