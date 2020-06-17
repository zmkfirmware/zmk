
#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
#include <zmk/keymap.h>
#include <dt-bindings/zmk/matrix-transform.h>
#include <sys/util.h>

static u32_t zmk_keymap_layer_state = 0;
static u8_t zmk_keymap_layer_default = 0;

#if DT_NODE_HAS_PROP(ZMK_KEYMAP_NODE, transform)
#define ZMK_KEYMAP_TRANSFORM_NODE DT_PHANDLE(ZMK_KEYMAP_NODE, transform)
#define ZMK_KEYMAP_LEN DT_PROP_LEN(ZMK_KEYMAP_TRANSFORM_NODE, map)

#define _TRANSFORM_ENTRY(i, l) \
	[(KT_ROW(DT_PROP_BY_IDX(ZMK_KEYMAP_TRANSFORM_NODE, map, i)) * ZMK_MATRIX_COLS) + KT_COL(DT_PROP_BY_IDX(ZMK_KEYMAP_TRANSFORM_NODE, map, i))] = DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(ZMK_KEYMAP_NODE, layers, l), keys, i),

#define TRANSFORMED_LAYER(idx) \
  { UTIL_LISTIFY(ZMK_KEYMAP_LEN, _TRANSFORM_ENTRY, idx) }

static zmk_key zmk_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_MATRIX_ROWS * ZMK_MATRIX_COLS] = {
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 0)
	TRANSFORMED_LAYER(0),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 1)
	TRANSFORMED_LAYER(1),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 2)
	TRANSFORMED_LAYER(2),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 3)
	TRANSFORMED_LAYER(3),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 4)
	TRANSFORMED_LAYER(4),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 5)
	TRANSFORMED_LAYER(5),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 6)
	TRANSFORMED_LAYER(6),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 7)
	TRANSFORMED_LAYER(7),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 8)
	TRANSFORMED_LAYER(8),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 9)
	TRANSFORMED_LAYER(9),
#endif
};

#else

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

#endif

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
			u8_t key_index = (row * ZMK_MATRIX_COLS) + column;
			LOG_DBG("Getting key at index %d", key_index);

			zmk_key key = zmk_keymap[layer][key_index];
			if (key == ZC_TRNS)
			{
				continue;
			}

			return key;
		}
	}

	return ZC_NO;
}
