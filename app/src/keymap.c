/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <sys/util.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix.h>
#include <zmk/sensors.h>
#include <zmk/keymap.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <zmk/event-manager.h>
#include <zmk/events/position-state-changed.h>
#include <zmk/events/sensor-event.h>

static u32_t zmk_keymap_layer_state = 0;
static u8_t zmk_keymap_layer_default = 0;

#define DT_DRV_COMPAT zmk_keymap

#define LAYER_CHILD_LEN(node) 1 +
#define ZMK_KEYMAP_NODE DT_DRV_INST(0)
#define ZMK_KEYMAP_LAYERS_LEN (DT_INST_FOREACH_CHILD(0, LAYER_CHILD_LEN) 0)

#define LAYER_NODE(l) DT_PHANDLE_BY_IDX(ZMK_KEYMAP_NODE, layers, l)

#define _TRANSFORM_ENTRY(idx, layer)                                                               \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_PHANDLE_BY_IDX(layer, bindings, idx)),                         \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(layer, bindings, idx, param1), (0),           \
                              (DT_PHA_BY_IDX(layer, bindings, idx, param1))),                      \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(layer, bindings, idx, param2), (0),           \
                              (DT_PHA_BY_IDX(layer, bindings, idx, param2))),                      \
    },

#define TRANSFORMED_LAYER(node) {UTIL_LISTIFY(DT_PROP_LEN(node, bindings), _TRANSFORM_ENTRY, node)},

#if ZMK_KEYMAP_HAS_SENSORS
#define _TRANSFORM_SENSOR_ENTRY(idx, layer)                                                        \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_PHANDLE_BY_IDX(layer, sensor_bindings, idx)),                  \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(layer, sensor_bindings, idx, param1), (0),    \
                              (DT_PHA_BY_IDX(layer, sensor_bindings, idx, param1))),               \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(layer, sensor_bindings, idx, param2), (0),    \
                              (DT_PHA_BY_IDX(layer, sensor_bindings, idx, param2))),               \
    },

#define SENSOR_LAYER(node)                                                                         \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_PROP(node, sensor_bindings),                                                   \
        ({UTIL_LISTIFY(DT_PROP_LEN(node, sensor_bindings), _TRANSFORM_SENSOR_ENTRY, node)}),       \
        ({})),

#endif /* ZMK_KEYMAP_HAS_SENSORS */

// State

// When a behavior handles a key position "down" event, we record the layer state
// here so that even if that layer is deactivated before the "up", event, we
// still send the release event to the behavior in that layer also.
static u32_t zmk_keymap_active_behavior_layer[ZMK_KEYMAP_LEN];

static struct zmk_behavior_binding zmk_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_KEYMAP_LEN] = {
    DT_INST_FOREACH_CHILD(0, TRANSFORMED_LAYER)};

#if ZMK_KEYMAP_HAS_SENSORS

static struct zmk_behavior_binding zmk_sensor_keymap[ZMK_KEYMAP_LAYERS_LEN]
                                                    [ZMK_KEYMAP_SENSORS_LEN] = {
                                                        DT_INST_FOREACH_CHILD(0, SENSOR_LAYER)};

#endif /* ZMK_KEYMAP_HAS_SENSORS */

#define SET_LAYER_STATE(layer, state)                                                              \
    if (layer >= 32) {                                                                             \
        return -EINVAL;                                                                            \
    }                                                                                              \
    WRITE_BIT(zmk_keymap_layer_state, layer, state);                                               \
    return 0;

bool zmk_keymap_layer_active(u8_t layer) {
    return (zmk_keymap_layer_state & (BIT(layer))) == (BIT(layer));
};

int zmk_keymap_layer_activate(u8_t layer) { SET_LAYER_STATE(layer, true); };

int zmk_keymap_layer_deactivate(u8_t layer) { SET_LAYER_STATE(layer, false); };

int zmk_keymap_layer_toggle(u8_t layer) {
    if (zmk_keymap_layer_active(layer)) {
        return zmk_keymap_layer_deactivate(layer);
    }

    return zmk_keymap_layer_activate(layer);
};

bool is_active_layer(u8_t layer, u32_t layer_state) {
    return (layer_state & BIT(layer)) == BIT(layer) || layer == zmk_keymap_layer_default;
}

int zmk_keymap_apply_position_state(int layer, u32_t position, bool pressed, s64_t timestamp) {
    struct zmk_behavior_binding *binding = &zmk_keymap[layer][position];
    struct device *behavior;
    struct zmk_behavior_binding_event event = {
        .layer = layer,
        .position = position,
        .timestamp = timestamp,
    };

    LOG_DBG("layer: %d position: %d, binding name: %s", layer, position,
            log_strdup(binding->behavior_dev));

    behavior = device_get_binding(binding->behavior_dev);

    if (!behavior) {
        LOG_DBG("No behavior assigned to %d on layer %d", position, layer);
        return 1;
    }

    if (pressed) {
        return behavior_keymap_binding_pressed(binding, event);
    } else {
        return behavior_keymap_binding_released(binding, event);
    }
}

int zmk_keymap_position_state_changed(u32_t position, bool pressed, s64_t timestamp) {
    if (pressed) {
        zmk_keymap_active_behavior_layer[position] = zmk_keymap_layer_state;
    }
    for (int layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer >= zmk_keymap_layer_default; layer--) {
        if (is_active_layer(layer, zmk_keymap_active_behavior_layer[position])) {
            int ret = zmk_keymap_apply_position_state(layer, position, pressed, timestamp);
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

#if ZMK_KEYMAP_HAS_SENSORS
int zmk_keymap_sensor_triggered(u8_t sensor_number, struct device *sensor, s64_t timestamp) {
    for (int layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer >= zmk_keymap_layer_default; layer--) {
        if (((zmk_keymap_layer_state & BIT(layer)) == BIT(layer) ||
             layer == zmk_keymap_layer_default) &&
            zmk_sensor_keymap[layer] != NULL) {
            struct zmk_behavior_binding *binding = &zmk_sensor_keymap[layer][sensor_number];
            struct device *behavior;
            int ret;

            LOG_DBG("layer: %d sensor_number: %d, binding name: %s", layer, sensor_number,
                    log_strdup(binding->behavior_dev));

            behavior = device_get_binding(binding->behavior_dev);

            if (!behavior) {
                LOG_DBG("No behavior assigned to %d on layer %d", sensor_number, layer);
                continue;
            }

            ret = behavior_sensor_keymap_binding_triggered(binding, sensor, timestamp);

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

#endif /* ZMK_KEYMAP_HAS_SENSORS */

int keymap_listener(const struct zmk_event_header *eh) {
    if (is_position_state_changed(eh)) {
        const struct position_state_changed *ev = cast_position_state_changed(eh);
        return zmk_keymap_position_state_changed(ev->position, ev->state, ev->timestamp);
#if ZMK_KEYMAP_HAS_SENSORS
    } else if (is_sensor_event(eh)) {
        const struct sensor_event *ev = cast_sensor_event(eh);
        return zmk_keymap_sensor_triggered(ev->sensor_number, ev->sensor, ev->timestamp);
#endif /* ZMK_KEYMAP_HAS_SENSORS */
    }

    return -ENOTSUP;
}

ZMK_LISTENER(keymap, keymap_listener);
ZMK_SUBSCRIPTION(keymap, position_state_changed);

#if ZMK_KEYMAP_HAS_SENSORS
ZMK_SUBSCRIPTION(keymap, sensor_event);
#endif /* ZMK_KEYMAP_HAS_SENSORS */
