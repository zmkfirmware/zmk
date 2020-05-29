
#include <zmk/keymap.h>

static u32_t zmk_keymap_layer_state = 0;
static u8_t zmk_keymap_layer_default = 0;

static zmk_key zmk_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_MATRIX_ROWS * ZMK_MATRIX_COLS] = {
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 0)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE, layers, 0, keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 1)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE, layers, 1, keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 2)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE, layers, 2, keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 3)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE, layers, 3, keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 4)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE, layers, 4, keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 5)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE, layers, 5, keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 6)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE, layers, 6, keys),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 7)
	DT_PROP_BY_PHANDLE_IDX(ZMK_KEYMAP_NODE, layers, 7, keys),
#endif
};

#define SET_LAYER_STATE(layer, state)                \
	if (layer >= 32)                                 \
	{                                                \
		return false;                                \
	}                                                \
	WRITE_BIT(zmk_keymap_layer_state, layer, state); \
	return true;

bool zmk_keymap_layer_activate(u8_t layer)
{
	SET_LAYER_STATE(layer, true);
};

bool zmk_keymap_layer_deactivate(u8_t layer)
{
	SET_LAYER_STATE(layer, false);
};

zmk_key zmk_keymap_keycode_from_position(u32_t row, u32_t column)
{
	for (int layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer >= zmk_keymap_layer_default; layer--)
	{
		if ((zmk_keymap_layer_state & BIT(layer)) == BIT(layer) || layer == zmk_keymap_layer_default)
		{
			zmk_key key = zmk_keymap[layer][(row * ZMK_MATRIX_COLS) + column];
			if (key == ZC_TRNS)
			{
				continue;
			}

			return key;
		}
	}

	return ZC_NO;
}
