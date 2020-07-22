
#include <sys/util.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix.h>
#include <zmk/sensors.h>
#include <zmk/keymap.h>
#include <dt-bindings/zmk/matrix-transform.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <zmk/event-manager.h>
#include <zmk/events/position-state-changed.h>
#include <zmk/events/sensor-event.h>

static u32_t zmk_keymap_layer_state = 0;
static u8_t zmk_keymap_layer_default = 0;

#define ZMK_KEYMAP_NODE DT_CHOSEN(zmk_keymap)
#define ZMK_KEYMAP_LAYERS_LEN DT_PROP_LEN(ZMK_KEYMAP_NODE, layers)

#define LAYER_NODE(l) DT_PHANDLE_BY_IDX(ZMK_KEYMAP_NODE, layers, l)

#define _TRANSFORM_ENTRY(idx, layer) \
	{ .behavior_dev = DT_LABEL(DT_PHANDLE_BY_IDX(DT_PHANDLE_BY_IDX(ZMK_KEYMAP_NODE, layers, layer), bindings, idx)), \
	  .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(LAYER_NODE(layer), bindings, idx, param1), (0), (DT_PHA_BY_IDX(LAYER_NODE(layer), bindings, idx, param1))), \
	  .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(LAYER_NODE(layer), bindings, idx, param2), (0), (DT_PHA_BY_IDX(LAYER_NODE(layer), bindings, idx, param2))), \
	},

#define TRANSFORMED_LAYER(idx) \
  { UTIL_LISTIFY(DT_PROP_LEN(DT_PHANDLE_BY_IDX(ZMK_KEYMAP_NODE, layers, idx), bindings), _TRANSFORM_ENTRY, idx) }

static struct zmk_behavior_binding zmk_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_KEYMAP_LEN] = {
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


#define _TRANSFORM_SENSOR_ENTRY(idx, layer) \
	{ .behavior_dev = DT_LABEL(DT_PHANDLE_BY_IDX(DT_PHANDLE_BY_IDX(ZMK_KEYMAP_NODE, layers, layer), sensor_bindings, idx)), \
	  .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(LAYER_NODE(layer), sensor_bindings, idx, param1), (0), (DT_PHA_BY_IDX(LAYER_NODE(layer), sensor_bindings, idx, param1))), \
	  .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(LAYER_NODE(layer), sensor_bindings, idx, param2), (0), (DT_PHA_BY_IDX(LAYER_NODE(layer), sensor_bindings, idx, param2))), \
	},

#define SENSOR_LAYER(idx) \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_PHANDLE_BY_IDX(ZMK_KEYMAP_NODE, layers, idx), sensor_bindings), \
		({ UTIL_LISTIFY(DT_PROP_LEN(DT_PHANDLE_BY_IDX(ZMK_KEYMAP_NODE, layers, idx), sensor_bindings), _TRANSFORM_SENSOR_ENTRY, idx) }), \
		(NULL))

static struct zmk_behavior_binding zmk_sensor_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_KEYMAP_SENSORS_LEN] = {
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 0)
	SENSOR_LAYER(0),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 1)
	SENSOR_LAYER(1),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 2)
	SENSOR_LAYER(2),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 3)
	SENSOR_LAYER(3),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 4)
	SENSOR_LAYER(4),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 5)
	SENSOR_LAYER(5),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 6)
	SENSOR_LAYER(6),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 7)
	SENSOR_LAYER(7),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 8)
	SENSOR_LAYER(8),
#endif
#if DT_PROP_HAS_IDX(ZMK_KEYMAP_NODE, layers, 9)
	SENSOR_LAYER(9),
#endif
};

#define SET_LAYER_STATE(layer, state)                \
	if (layer >= 32)                                 \
	{                                                \
		return -EINVAL;                              \
	}                                                \
	WRITE_BIT(zmk_keymap_layer_state, layer, state); \
	return 0;

int zmk_keymap_layer_activate(u8_t layer)
{
	SET_LAYER_STATE(layer, true);
};

int zmk_keymap_layer_deactivate(u8_t layer)
{
	SET_LAYER_STATE(layer, false);
};

int zmk_keymap_position_state_changed(u32_t position, bool pressed)
{
	for (int layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer >= zmk_keymap_layer_default; layer--)
	{
		if ((zmk_keymap_layer_state & BIT(layer)) == BIT(layer) || layer == zmk_keymap_layer_default)
		{
			struct zmk_behavior_binding *binding = &zmk_keymap[layer][position];
			struct device *behavior;
			int ret;

			LOG_DBG("layer: %d position: %d, binding name: %s", layer, position, log_strdup(binding->behavior_dev));

			behavior = device_get_binding(binding->behavior_dev);

			if (!behavior) {
				LOG_DBG("No behavior assigned to %d on layer %d", position, layer);
				continue;
			}
			if (pressed) {
				ret = behavior_keymap_binding_pressed(behavior, position, binding->param1, binding->param2);
			} else {
				ret = behavior_keymap_binding_released(behavior, position, binding->param1, binding->param2);
			}
			

			if (ret > 0) {
				LOG_DBG("behavior processing to continue to next layer");
				continue;
			} else if (ret < 0) {
				LOG_DBG("Behavior returned error: %d", ret);
				return ret; 
			} else {
				return ret;
			}
		}
	}

	return -ENOTSUP;
}

int zmk_keymap_sensor_triggered(u8_t sensor_number, struct device *sensor)
{
	for (int layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer >= zmk_keymap_layer_default; layer--)
	{
		if (((zmk_keymap_layer_state & BIT(layer)) == BIT(layer) || layer == zmk_keymap_layer_default) && zmk_sensor_keymap[layer] != NULL)
		{
			struct zmk_behavior_binding *binding = &zmk_sensor_keymap[layer][sensor_number];
			struct device *behavior;
			int ret;

			LOG_DBG("layer: %d sensor_number: %d, binding name: %s", layer, sensor_number, log_strdup(binding->behavior_dev));

			behavior = device_get_binding(binding->behavior_dev);

			if (!behavior) {
				LOG_DBG("No behavior assigned to %d on layer %d", sensor_number, layer);
				continue;
			}
			
			ret = behavior_sensor_keymap_binding_triggered(behavior, sensor, binding->param1, binding->param2);

			if (ret > 0) {
				LOG_DBG("behavior processing to continue to next layer");
				continue;
			} else if (ret < 0) {
				LOG_DBG("Behavior returned error: %d", ret);
				return ret; 
			} else {
				return ret;
			}
		}
	}

	return -ENOTSUP;
}

int keymap_listener(const struct zmk_event_header *eh)
{
	if (is_position_state_changed(eh)) {
		const struct position_state_changed *ev = cast_position_state_changed(eh);
		return zmk_keymap_position_state_changed(ev->position, ev->state);
	} else if (is_sensor_event(eh)) {
		const struct sensor_event *ev = cast_sensor_event(eh);
		return zmk_keymap_sensor_triggered(ev->sensor_number, ev->sensor);
	}

	return -ENOTSUP;
}

ZMK_LISTENER(keymap, keymap_listener);
ZMK_SUBSCRIPTION(keymap, position_state_changed);
ZMK_SUBSCRIPTION(keymap, sensor_event);

