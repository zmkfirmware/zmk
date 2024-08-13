/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>
#include <zmk/sensors.h>
#include <zmk/virtual_key_position.h>

#include <zmk/ble.h>
#if ZMK_BLE_IS_CENTRAL
#include <zmk/split/bluetooth/central.h>
#endif

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/sensor_event.h>

static zmk_keymap_layers_state_t _zmk_keymap_layer_state = 0;
static uint8_t _zmk_keymap_layer_default = 0;

#define DT_DRV_COMPAT zmk_keymap

#if !DT_NODE_EXISTS(DT_DRV_INST(0))

#error "Keymap node not found, check a keymap is available and is has compatible = "zmk,keymap" set"

#endif

#define TRANSFORMED_LAYER(node)                                                                    \
    { LISTIFY(DT_PROP_LEN(node, bindings), ZMK_KEYMAP_EXTRACT_BINDING, (, ), node) }

#if ZMK_KEYMAP_HAS_SENSORS
#define _TRANSFORM_SENSOR_ENTRY(idx, layer)                                                        \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(layer, sensor_bindings, idx)),            \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(layer, sensor_bindings, idx, param1), (0),    \
                              (DT_PHA_BY_IDX(layer, sensor_bindings, idx, param1))),               \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(layer, sensor_bindings, idx, param2), (0),    \
                              (DT_PHA_BY_IDX(layer, sensor_bindings, idx, param2))),               \
    }

#define SENSOR_LAYER(node)                                                                         \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_PROP(node, sensor_bindings),                                                   \
        ({LISTIFY(DT_PROP_LEN(node, sensor_bindings), _TRANSFORM_SENSOR_ENTRY, (, ), node)}),      \
        ({}))

#endif /* ZMK_KEYMAP_HAS_SENSORS */

#define LAYER_NAME(node) DT_PROP_OR(node, display_name, DT_PROP_OR(node, label, NULL))

// State

// When a behavior handles a key position "down" event, we record the layer state
// here so that even if that layer is deactivated before the "up", event, we
// still send the release event to the behavior in that layer also.
static uint32_t zmk_keymap_active_behavior_layer[ZMK_KEYMAP_LEN];

static struct zmk_behavior_binding zmk_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_KEYMAP_LEN] = {
    DT_INST_FOREACH_CHILD_SEP(0, TRANSFORMED_LAYER, (, ))};

static const char *zmk_keymap_layer_names[ZMK_KEYMAP_LAYERS_LEN] = {
    DT_INST_FOREACH_CHILD_SEP(0, LAYER_NAME, (, ))};

#if ZMK_KEYMAP_HAS_SENSORS

static struct zmk_behavior_binding
    zmk_sensor_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_KEYMAP_SENSORS_LEN] = {
        DT_INST_FOREACH_CHILD_SEP(0, SENSOR_LAYER, (, ))};

#endif /* ZMK_KEYMAP_HAS_SENSORS */

static inline int set_layer_state(uint8_t layer, bool state) {
    int ret = 0;
    if (layer >= ZMK_KEYMAP_LAYERS_LEN) {
        return -EINVAL;
    }

    // Default layer should *always* remain active
    if (layer == _zmk_keymap_layer_default && !state) {
        return 0;
    }

    zmk_keymap_layers_state_t old_state = _zmk_keymap_layer_state;
    WRITE_BIT(_zmk_keymap_layer_state, layer, state);
    // Don't send state changes unless there was an actual change
    if (old_state != _zmk_keymap_layer_state) {
        LOG_DBG("layer_changed: layer %d state %d", layer, state);
        ret = raise_layer_state_changed(layer, state);
        if (ret < 0) {
            LOG_WRN("Failed to raise layer state changed (%d)", ret);
        }
    }

    return ret;
}

uint8_t zmk_keymap_layer_default(void) { return _zmk_keymap_layer_default; }

zmk_keymap_layers_state_t zmk_keymap_layer_state(void) { return _zmk_keymap_layer_state; }

bool zmk_keymap_layer_active_with_state(uint8_t layer, zmk_keymap_layers_state_t state_to_test) {
    // The default layer is assumed to be ALWAYS ACTIVE so we include an || here to ensure nobody
    // breaks up that assumption by accident
    return (state_to_test & (BIT(layer))) == (BIT(layer)) || layer == _zmk_keymap_layer_default;
};

bool zmk_keymap_layer_active(uint8_t layer) {
    return zmk_keymap_layer_active_with_state(layer, _zmk_keymap_layer_state);
};

uint8_t zmk_keymap_highest_layer_active(void) {
    for (uint8_t layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer > 0; layer--) {
        if (zmk_keymap_layer_active(layer)) {
            return layer;
        }
    }
    return zmk_keymap_layer_default();
}

int zmk_keymap_layer_activate(uint8_t layer) { return set_layer_state(layer, true); };

int zmk_keymap_layer_deactivate(uint8_t layer) { return set_layer_state(layer, false); };

int zmk_keymap_layer_toggle(uint8_t layer) {
    if (zmk_keymap_layer_active(layer)) {
        return zmk_keymap_layer_deactivate(layer);
    }

    return zmk_keymap_layer_activate(layer);
};

int zmk_keymap_layer_to(uint8_t layer) {
    for (int i = ZMK_KEYMAP_LAYERS_LEN - 1; i >= 0; i--) {
        zmk_keymap_layer_deactivate(i);
    }

    zmk_keymap_layer_activate(layer);

    return 0;
}

bool is_active_layer(uint8_t layer, zmk_keymap_layers_state_t layer_state) {
    return (layer_state & BIT(layer)) == BIT(layer) || layer == _zmk_keymap_layer_default;
}

const char *zmk_keymap_layer_name(uint8_t layer) {
    if (layer >= ZMK_KEYMAP_LAYERS_LEN) {
        return NULL;
    }

    return zmk_keymap_layer_names[layer];
}

int invoke_locally(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
                   bool pressed) {
    if (pressed) {
        return behavior_keymap_binding_pressed(binding, event);
    } else {
        return behavior_keymap_binding_released(binding, event);
    }
}

int zmk_keymap_apply_position_state(uint8_t source, int layer, uint32_t position, bool pressed,
                                    int64_t timestamp) {
    // We want to make a copy of this, since it may be converted from
    // relative to absolute before being invoked
    struct zmk_behavior_binding binding = zmk_keymap[layer][position];
    const struct device *behavior;
    struct zmk_behavior_binding_event event = {
        .layer = layer,
        .position = position,
        .timestamp = timestamp,
    };

    LOG_DBG("layer: %d position: %d, binding name: %s", layer, position, binding.behavior_dev);

    behavior = zmk_behavior_get_binding(binding.behavior_dev);

    if (!behavior) {
        LOG_WRN("No behavior assigned to %d on layer %d", position, layer);
        return 1;
    }

    int err = behavior_keymap_binding_convert_central_state_dependent_params(&binding, event);
    if (err) {
        LOG_ERR("Failed to convert relative to absolute behavior binding (err %d)", err);
        return err;
    }

    enum behavior_locality locality = BEHAVIOR_LOCALITY_CENTRAL;
    err = behavior_get_locality(behavior, &locality);
    if (err) {
        LOG_ERR("Failed to get behavior locality %d", err);
        return err;
    }

    switch (locality) {
    case BEHAVIOR_LOCALITY_CENTRAL:
        return invoke_locally(&binding, event, pressed);
    case BEHAVIOR_LOCALITY_EVENT_SOURCE:
#if ZMK_BLE_IS_CENTRAL
        if (source == ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL) {
            return invoke_locally(&binding, event, pressed);
        } else {
            return zmk_split_bt_invoke_behavior(source, &binding, event, pressed);
        }
#else
        return invoke_locally(&binding, event, pressed);
#endif
    case BEHAVIOR_LOCALITY_GLOBAL:
#if ZMK_BLE_IS_CENTRAL
        for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
            zmk_split_bt_invoke_behavior(i, &binding, event, pressed);
        }
#endif
        return invoke_locally(&binding, event, pressed);
    }

    return -ENOTSUP;
}

int zmk_keymap_position_state_changed(uint8_t source, uint32_t position, bool pressed,
                                      int64_t timestamp) {
    if (pressed) {
        zmk_keymap_active_behavior_layer[position] = _zmk_keymap_layer_state;
    }
    for (int layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer >= _zmk_keymap_layer_default; layer--) {
        if (zmk_keymap_layer_active_with_state(layer, zmk_keymap_active_behavior_layer[position])) {
            int ret = zmk_keymap_apply_position_state(source, layer, position, pressed, timestamp);
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
int zmk_keymap_sensor_event(uint8_t sensor_index,
                            const struct zmk_sensor_channel_data *channel_data,
                            size_t channel_data_size, int64_t timestamp) {
    bool opaque_response = false;

    for (int layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer >= 0; layer--) {
        struct zmk_behavior_binding *binding = &zmk_sensor_keymap[layer][sensor_index];

        LOG_DBG("layer: %d sensor_index: %d, binding name: %s", layer, sensor_index,
                binding->behavior_dev);

        const struct device *behavior = zmk_behavior_get_binding(binding->behavior_dev);
        if (!behavior) {
            LOG_DBG("No behavior assigned to %d on layer %d", sensor_index, layer);
            continue;
        }

        struct zmk_behavior_binding_event event = {
            .layer = layer,
            .position = ZMK_VIRTUAL_KEY_POSITION_SENSOR(sensor_index),
            .timestamp = timestamp,
        };

        int ret = behavior_sensor_keymap_binding_accept_data(
            binding, event, zmk_sensors_get_config_at_index(sensor_index), channel_data_size,
            channel_data);

        if (ret < 0) {
            LOG_WRN("behavior data accept for behavior %s returned an error (%d). Processing to "
                    "continue to next layer",
                    binding->behavior_dev, ret);
            continue;
        }

        enum behavior_sensor_binding_process_mode mode =
            (!opaque_response && layer >= _zmk_keymap_layer_default &&
             zmk_keymap_layer_active(layer))
                ? BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_TRIGGER
                : BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_DISCARD;

        ret = behavior_sensor_keymap_binding_process(binding, event, mode);

        if (ret == ZMK_BEHAVIOR_OPAQUE) {
            LOG_DBG("sensor event processing complete, behavior response was opaque");
            opaque_response = true;
        } else if (ret < 0) {
            LOG_DBG("Behavior returned error: %d", ret);
            return ret;
        }
    }

    return 0;
}

#endif /* ZMK_KEYMAP_HAS_SENSORS */

int keymap_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *pos_ev;
    if ((pos_ev = as_zmk_position_state_changed(eh)) != NULL) {
        return zmk_keymap_position_state_changed(pos_ev->source, pos_ev->position, pos_ev->state,
                                                 pos_ev->timestamp);
    }

#if ZMK_KEYMAP_HAS_SENSORS
    const struct zmk_sensor_event *sensor_ev;
    if ((sensor_ev = as_zmk_sensor_event(eh)) != NULL) {
        return zmk_keymap_sensor_event(sensor_ev->sensor_index, sensor_ev->channel_data,
                                       sensor_ev->channel_data_size, sensor_ev->timestamp);
    }
#endif /* ZMK_KEYMAP_HAS_SENSORS */

    return -ENOTSUP;
}

ZMK_LISTENER(keymap, keymap_listener);
ZMK_SUBSCRIPTION(keymap, zmk_position_state_changed);

#if ZMK_KEYMAP_HAS_SENSORS
ZMK_SUBSCRIPTION(keymap, zmk_sensor_event);
#endif /* ZMK_KEYMAP_HAS_SENSORS */
