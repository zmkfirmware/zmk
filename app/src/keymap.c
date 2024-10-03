/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <zephyr/sys/util.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/stdlib.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>
#include <zmk/sensors.h>
#include <zmk/virtual_key_position.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/sensor_event.h>

static zmk_keymap_layers_state_t _zmk_keymap_layer_state = 0;
static zmk_keymap_layer_id_t _zmk_keymap_layer_default = 0;

#define DT_DRV_COMPAT zmk_keymap

#if !DT_NODE_EXISTS(DT_DRV_INST(0))

#error "Keymap node not found, check a keymap is available and is has compatible = "zmk,keymap" set"

#endif

#define TRANSFORMED_LAYER(node)                                                                    \
    {COND_CODE_1(DT_NODE_HAS_PROP(node, bindings),                                                 \
                 (LISTIFY(DT_PROP_LEN(node, bindings), ZMK_KEYMAP_EXTRACT_BINDING, (, ), node)),   \
                 ())}

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

#define LAYER_NAME(node) DT_PROP_OR(node, display_name, DT_PROP_OR(node, label, ""))

// State

// When a behavior handles a key position "down" event, we record the layer state
// here so that even if that layer is deactivated before the "up", event, we
// still send the release event to the behavior in that layer also.
static uint32_t zmk_keymap_active_behavior_layer[ZMK_KEYMAP_LEN];

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

static uint8_t keymap_layer_orders[ZMK_KEYMAP_LAYERS_LEN];

#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

#define KEYMAP_VAR(_name, _opts)                                                                   \
    static _opts struct zmk_behavior_binding _name[ZMK_KEYMAP_LAYERS_LEN][ZMK_KEYMAP_LEN] = {      \
        COND_CODE_1(IS_ENABLED(CONFIG_ZMK_STUDIO),                                                 \
                    (DT_INST_FOREACH_CHILD_SEP(0, TRANSFORMED_LAYER, (, ))),                       \
                    (DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(0, TRANSFORMED_LAYER, (, ))))};

KEYMAP_VAR(zmk_keymap, COND_CODE_1(IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE), (), (const)))

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)

KEYMAP_VAR(zmk_stock_keymap, const)

static char zmk_keymap_layer_names[ZMK_KEYMAP_LAYERS_LEN][CONFIG_ZMK_KEYMAP_LAYER_NAME_MAX_LEN] = {
    DT_INST_FOREACH_CHILD_SEP(0, LAYER_NAME, (, ))};

static uint32_t changed_layer_names = 0;

#else

static const char *zmk_keymap_layer_names[ZMK_KEYMAP_LAYERS_LEN] = {
    DT_INST_FOREACH_CHILD_SEP(0, LAYER_NAME, (, ))};

#endif

#if ZMK_KEYMAP_HAS_SENSORS

static struct zmk_behavior_binding
    zmk_sensor_keymap[ZMK_KEYMAP_LAYERS_LEN][ZMK_KEYMAP_SENSORS_LEN] = {
        DT_INST_FOREACH_CHILD_SEP(0, SENSOR_LAYER, (, ))};

#endif /* ZMK_KEYMAP_HAS_SENSORS */

#define ASSERT_LAYER_VAL(_layer, _fail_ret)                                                        \
    if ((_layer) >= ZMK_KEYMAP_LAYERS_LEN) {                                                       \
        return (_fail_ret);                                                                        \
    }

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

uint8_t map_layer_id_to_index(zmk_keymap_layer_id_t layer_id) {
    for (uint8_t i = 0; i < ZMK_KEYMAP_LAYERS_LEN; i++) {
        if (keymap_layer_orders[i] == layer_id) {
            return i;
        }
    }

    return ZMK_KEYMAP_LAYER_ID_INVAL;
}

#define LAYER_INDEX_TO_ID(_layer) keymap_layer_orders[_layer]
#define LAYER_ID_TO_INDEX(_layer) map_layer_id_to_index(_layer)

#else

#define LAYER_INDEX_TO_ID(_layer) _layer
#define LAYER_ID_TO_INDEX(_layer) _layer

#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

static inline int set_layer_state(zmk_keymap_layer_id_t layer_id, bool state) {
    int ret = 0;
    if (layer_id >= ZMK_KEYMAP_LAYERS_LEN) {
        return -EINVAL;
    }

    // Default layer should *always* remain active
    if (layer_id == _zmk_keymap_layer_default && !state) {
        return 0;
    }

    zmk_keymap_layers_state_t old_state = _zmk_keymap_layer_state;
    WRITE_BIT(_zmk_keymap_layer_state, layer_id, state);
    // Don't send state changes unless there was an actual change
    if (old_state != _zmk_keymap_layer_state) {
        LOG_DBG("layer_changed: layer %d state %d", layer_id, state);
        ret = raise_layer_state_changed(layer_id, state);
        if (ret < 0) {
            LOG_WRN("Failed to raise layer state changed (%d)", ret);
        }
    }

    return ret;
}

zmk_keymap_layer_id_t zmk_keymap_layer_index_to_id(zmk_keymap_layer_index_t layer_index) {
    ASSERT_LAYER_VAL(layer_index, UINT8_MAX);

    return LAYER_INDEX_TO_ID(layer_index);
}

zmk_keymap_layer_id_t zmk_keymap_layer_default(void) { return _zmk_keymap_layer_default; }

zmk_keymap_layers_state_t zmk_keymap_layer_state(void) { return _zmk_keymap_layer_state; }

bool zmk_keymap_layer_active_with_state(zmk_keymap_layer_id_t layer,
                                        zmk_keymap_layers_state_t state_to_test) {
    // The default layer is assumed to be ALWAYS ACTIVE so we include an || here to ensure nobody
    // breaks up that assumption by accident
    return (state_to_test & (BIT(layer))) == (BIT(layer)) || layer == _zmk_keymap_layer_default;
};

bool zmk_keymap_layer_active(zmk_keymap_layer_id_t layer) {
    return zmk_keymap_layer_active_with_state(layer, _zmk_keymap_layer_state);
};

zmk_keymap_layer_index_t zmk_keymap_highest_layer_active(void) {
    for (int layer_idx = ZMK_KEYMAP_LAYERS_LEN - 1;
         layer_idx >= LAYER_ID_TO_INDEX(_zmk_keymap_layer_default); layer_idx--) {
        zmk_keymap_layer_id_t layer_id = LAYER_INDEX_TO_ID(layer_idx);

        if (layer_id == ZMK_KEYMAP_LAYER_ID_INVAL) {
            continue;
        }
        if (zmk_keymap_layer_active(layer_id)) {
            return LAYER_ID_TO_INDEX(layer_id);
        }
    }

    return LAYER_ID_TO_INDEX(zmk_keymap_layer_default());
}

int zmk_keymap_layer_activate(zmk_keymap_layer_id_t layer) { return set_layer_state(layer, true); };

int zmk_keymap_layer_deactivate(zmk_keymap_layer_id_t layer) {
    return set_layer_state(layer, false);
};

int zmk_keymap_layer_toggle(zmk_keymap_layer_id_t layer) {
    if (zmk_keymap_layer_active(layer)) {
        return zmk_keymap_layer_deactivate(layer);
    }

    return zmk_keymap_layer_activate(layer);
};

int zmk_keymap_layer_to(zmk_keymap_layer_id_t layer) {
    for (int i = ZMK_KEYMAP_LAYERS_LEN - 1; i >= 0; i--) {
        zmk_keymap_layer_deactivate(i);
    }

    zmk_keymap_layer_activate(layer);

    return 0;
}

const char *zmk_keymap_layer_name(zmk_keymap_layer_id_t layer_id) {
    ASSERT_LAYER_VAL(layer_id, NULL)

    return zmk_keymap_layer_names[layer_id];
}

const struct zmk_behavior_binding *
zmk_keymap_get_layer_binding_at_idx(zmk_keymap_layer_id_t layer_id, uint8_t binding_idx) {
    if (binding_idx >= ZMK_KEYMAP_LEN) {
        return NULL;
    }

    ASSERT_LAYER_VAL(layer_id, NULL)

    return &zmk_keymap[layer_id][binding_idx];
}

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)

#define PENDING_ARRAY_SIZE DIV_ROUND_UP(ZMK_KEYMAP_LEN, 8)

static uint8_t zmk_keymap_layer_pending_changes[ZMK_KEYMAP_LAYERS_LEN][PENDING_ARRAY_SIZE];

int zmk_keymap_set_layer_binding_at_idx(zmk_keymap_layer_id_t layer_id, uint8_t binding_idx,
                                        struct zmk_behavior_binding binding) {
    if (binding_idx >= ZMK_KEYMAP_LEN) {
        return -EINVAL;
    }

    ASSERT_LAYER_VAL(layer_id, -EINVAL)

    uint8_t *pending = zmk_keymap_layer_pending_changes[layer_id];

    WRITE_BIT(pending[binding_idx / 8], binding_idx % 8, 1);

    // TODO: Need a mutex to protect access to the keymap data?
    memcpy(&zmk_keymap[layer_id][binding_idx], &binding, sizeof(binding));

    return 0;
}

#else

int zmk_keymap_set_layer_binding_at_idx(zmk_keymap_layer_id_t layer_id, uint8_t binding_idx,
                                        struct zmk_behavior_binding binding) {
    return -ENOTSUP;
}

#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)
#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)

static uint8_t settings_layer_orders[ZMK_KEYMAP_LAYERS_LEN];

#endif

int zmk_keymap_move_layer(zmk_keymap_layer_index_t start_idx, zmk_keymap_layer_index_t dest_idx) {
    ASSERT_LAYER_VAL(start_idx, -EINVAL)
    ASSERT_LAYER_VAL(dest_idx, -EINVAL)

    if (start_idx == dest_idx) {
        return 0;
    } else if (dest_idx > start_idx) {
        uint8_t val = keymap_layer_orders[start_idx];

        for (int i = start_idx; i < dest_idx; i++) {
            keymap_layer_orders[i] = keymap_layer_orders[i + 1];
        }

        keymap_layer_orders[dest_idx] = val;
    } else {
        uint8_t val = keymap_layer_orders[start_idx];

        for (int i = start_idx; i > dest_idx; i--) {
            keymap_layer_orders[i] = keymap_layer_orders[i - 1];
        }

        keymap_layer_orders[dest_idx] = val;
    }

    return 0;
}

int zmk_keymap_add_layer(void) {
    uint32_t seen_layer_ids = 0;
    LOG_HEXDUMP_DBG(keymap_layer_orders, ZMK_KEYMAP_LAYERS_LEN, "Order");

    for (int index = 0; index < ZMK_KEYMAP_LAYERS_LEN; index++) {
        zmk_keymap_layer_id_t id = LAYER_INDEX_TO_ID(index);

        if (id != ZMK_KEYMAP_LAYER_ID_INVAL) {
            WRITE_BIT(seen_layer_ids, id, 1);
            continue;
        }

        for (int candidate_id = 0; candidate_id < ZMK_KEYMAP_LAYERS_LEN; candidate_id++) {
            if (!(seen_layer_ids & BIT(candidate_id))) {
                keymap_layer_orders[index] = candidate_id;
                return index;
            }
        }
    }

    return -ENOSPC;
}

int zmk_keymap_remove_layer(zmk_keymap_layer_index_t index) {
    ASSERT_LAYER_VAL(index, -EINVAL);

    if (keymap_layer_orders[index] == ZMK_KEYMAP_LAYER_ID_INVAL) {
        return -EINVAL;
    }

    LOG_DBG("Removing layer index %d which is ID %d", index, keymap_layer_orders[index]);
    LOG_HEXDUMP_DBG(keymap_layer_orders, ZMK_KEYMAP_LAYERS_LEN, "Order");

    while (index < ZMK_KEYMAP_LAYERS_LEN - 1) {
        keymap_layer_orders[index] = keymap_layer_orders[index + 1];
        index++;
    }

    keymap_layer_orders[ZMK_KEYMAP_LAYERS_LEN - 1] = ZMK_KEYMAP_LAYER_ID_INVAL;

    LOG_HEXDUMP_DBG(keymap_layer_orders, ZMK_KEYMAP_LAYERS_LEN, "Order");

    return 0;
}

int zmk_keymap_restore_layer(zmk_keymap_layer_id_t id, zmk_keymap_layer_index_t at_index) {
    ASSERT_LAYER_VAL(at_index, -EINVAL);
    ASSERT_LAYER_VAL(id, -ENODEV);

    for (zmk_keymap_layer_index_t index = ZMK_KEYMAP_LAYERS_LEN - 1; index > at_index; index--) {
        keymap_layer_orders[index] = keymap_layer_orders[index - 1];
    }

    keymap_layer_orders[at_index] = id;

    return 0;
}

int zmk_keymap_set_layer_name(zmk_keymap_layer_id_t id, const char *name, size_t size) {
    ASSERT_LAYER_VAL(id, -EINVAL);

    if (size >= CONFIG_ZMK_KEYMAP_LAYER_NAME_MAX_LEN) {
        return -ENOSPC;
    }

    strlcpy(zmk_keymap_layer_names[id], name, CONFIG_ZMK_KEYMAP_LAYER_NAME_MAX_LEN);

    // Ensure we properly null terminate our name if we previously had a longer one.
    if (size < CONFIG_ZMK_KEYMAP_LAYER_NAME_MAX_LEN - 1) {
        zmk_keymap_layer_names[id][size] = 0;
    }

    WRITE_BIT(changed_layer_names, id, 1);

    return 0;
}

#else

int zmk_keymap_move_layer(zmk_keymap_layer_index_t layer, zmk_keymap_layer_index_t dest) {
    return -ENOTSUP;
}

int zmk_keymap_add_layer(void) { return -ENOTSUP; }

int zmk_keymap_remove_layer(zmk_keymap_layer_index_t index) { return -ENOTSUP; }

int zmk_keymap_restore_layer(zmk_keymap_layer_id_t id, zmk_keymap_layer_index_t at_index) {
    return -ENOTSUP;
}

int zmk_keymap_set_layer_name(zmk_keymap_layer_id_t id, const char *name, size_t size) {
    return -ENOTSUP;
}

#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)

#define PENDING_ARRAY_SIZE DIV_ROUND_UP(ZMK_KEYMAP_LEN, 8)

static uint8_t zmk_keymap_layer_pending_changes[ZMK_KEYMAP_LAYERS_LEN][PENDING_ARRAY_SIZE];

struct zmk_behavior_binding_setting {
    zmk_behavior_local_id_t behavior_local_id;
    uint32_t param1;
    uint32_t param2;
} __packed;

int zmk_keymap_check_unsaved_changes(void) {
    for (int l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
        uint8_t *pending = zmk_keymap_layer_pending_changes[l];
        for (int kp = 0; kp < ZMK_KEYMAP_LEN; kp++) {
            if (pending[kp / 8] & BIT(kp % 8)) {
                return 1;
            }
        }

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)
        if (settings_layer_orders[l] != keymap_layer_orders[l]) {
            return 1;
        }
#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)
    }

    return 0;
}

#define LAYER_ORDER_SETTINGS_KEY "keymap/layer_order"
#define LAYER_NAME_SETTINGS_KEY "keymap/l_n/%d"
#define LAYER_BINDING_SETTINGS_KEY "keymap/l/%d/%d"

static int save_bindings(void) {
    for (int l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
        uint8_t *pending = zmk_keymap_layer_pending_changes[l];

        for (int kp = 0; kp < ZMK_KEYMAP_LEN; kp++) {
            if (pending[kp / 8] & BIT(kp % 8)) {
                LOG_DBG("Pending save for layer %d at key position %d", l, kp);

                struct zmk_behavior_binding *binding = &zmk_keymap[l][kp];
                struct zmk_behavior_binding_setting binding_setting = {
                    .behavior_local_id = zmk_behavior_get_local_id(binding->behavior_dev),
                    .param1 = binding->param1,
                    .param2 = binding->param2,
                };

                // We can skip any trailing zero params, regardless of the behavior
                // and if those params are meaningful.
                size_t len = sizeof(binding_setting);
                if (binding_setting.param2 == 0) {
                    len -= 4;

                    if (binding_setting.param1 == 0) {
                        len -= 4;
                    }
                }

                char setting_name[20];
                sprintf(setting_name, LAYER_BINDING_SETTINGS_KEY, l, kp);

                int ret = settings_save_one(setting_name, &binding_setting, len);
                if (ret < 0) {
                    LOG_ERR("Failed to save keymap binding at %d on layer %d (%d)", l, kp, ret);
                    return ret;
                }
            }
        }

        *pending = 0;
    }

    return 0;
}

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)
static int save_layer_orders(void) {
    int ret = settings_save_one(LAYER_ORDER_SETTINGS_KEY, keymap_layer_orders,
                                ARRAY_SIZE(keymap_layer_orders));
    if (ret < 0) {
        return ret;
    }

    memcpy(settings_layer_orders, keymap_layer_orders, ARRAY_SIZE(keymap_layer_orders));
    return 0;
}
#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

static int save_layer_names(void) {
    for (int id = 0; id < ZMK_KEYMAP_LAYERS_LEN; id++) {
        if (changed_layer_names & BIT(id)) {
            char setting_name[14];
            sprintf(setting_name, LAYER_NAME_SETTINGS_KEY, id);
            int ret = settings_save_one(setting_name, zmk_keymap_layer_names[id],
                                        strlen(zmk_keymap_layer_names[id]));
            if (ret < 0) {
                return ret;
            }
        }
    }

    changed_layer_names = 0;
    return 0;
}

int zmk_keymap_save_changes(void) {
    int ret = save_bindings();
    if (ret < 0) {
        return ret;
    }

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)
    ret = save_layer_orders();
    if (ret < 0) {
        return ret;
    }
#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

    return save_layer_names();
}

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

#define KEYMAP_LAYER_ORDER_INIT(n)                                                                 \
    keymap_layer_orders[i] = i;                                                                    \
    settings_layer_orders[i] = i;                                                                  \
    i++;

static void load_stock_keymap_layer_ordering() {
    int i = 0;
    DT_INST_FOREACH_CHILD_STATUS_OKAY(0, KEYMAP_LAYER_ORDER_INIT)
    while (i < ZMK_KEYMAP_LAYERS_LEN) {
        keymap_layer_orders[i] = ZMK_KEYMAP_LAYER_ID_INVAL;
        i++;
    }
}
#endif

static void reload_from_stock_keymap(void) {
    for (int l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
        for (int k = 0; k < ZMK_KEYMAP_LEN; k++) {
            zmk_keymap[l][k] = zmk_stock_keymap[l][k];
        }
    }
}

int zmk_keymap_discard_changes(void) {
    load_stock_keymap_layer_ordering();
    reload_from_stock_keymap();

    int ret = settings_load_subtree("keymap");
    if (ret >= 0) {
        changed_layer_names = 0;

        for (int l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
            memset(zmk_keymap_layer_pending_changes[l], 0, PENDING_ARRAY_SIZE);
        }
    }

    return ret;
}

int zmk_keymap_reset_settings(void) {
    settings_delete(LAYER_ORDER_SETTINGS_KEY);
    for (int l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
        char layer_name_setting_name[14];
        sprintf(layer_name_setting_name, LAYER_NAME_SETTINGS_KEY, l);
        settings_delete(layer_name_setting_name);

        for (int k = 0; k < ZMK_KEYMAP_LEN; k++) {
            if (memcmp(&zmk_keymap[l][k], &zmk_stock_keymap[l][k],
                       sizeof(struct zmk_behavior_binding_setting)) == 0) {
                continue;
            }

            char setting_name[20];
            sprintf(setting_name, LAYER_BINDING_SETTINGS_KEY, l, k);
            settings_delete(setting_name);
        }
    }

    load_stock_keymap_layer_ordering();

    reload_from_stock_keymap();

    return 0;
}

#else

int zmk_keymap_save_changes(void) { return -ENOTSUP; }

int zmk_keymap_discard_changes(void) { return -ENOTSUP; }

int zmk_keymap_reset_settings(void) { return -ENOTSUP; }

#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)

int zmk_keymap_apply_position_state(uint8_t source, zmk_keymap_layer_id_t layer_id,
                                    uint32_t position, bool pressed, int64_t timestamp) {
    const struct zmk_behavior_binding *binding = &zmk_keymap[layer_id][position];
    struct zmk_behavior_binding_event event = {
        .layer = layer_id,
        .position = position,
        .timestamp = timestamp,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = source,
#endif
    };

    LOG_DBG("layer_id: %d position: %d, binding name: %s", layer_id, position,
            binding->behavior_dev);

    return zmk_behavior_invoke_binding(binding, event, pressed);
}

int zmk_keymap_position_state_changed(uint8_t source, uint32_t position, bool pressed,
                                      int64_t timestamp) {
    if (pressed) {
        zmk_keymap_active_behavior_layer[position] = _zmk_keymap_layer_state;
    }

    // We use int here to be sure we don't loop layer_idx back to UINT8_MAX
    for (int layer_idx = ZMK_KEYMAP_LAYERS_LEN - 1;
         layer_idx >= LAYER_ID_TO_INDEX(_zmk_keymap_layer_default); layer_idx--) {
        zmk_keymap_layer_id_t layer_id = LAYER_INDEX_TO_ID(layer_idx);

        if (layer_id == ZMK_KEYMAP_LAYER_ID_INVAL) {
            continue;
        }
        if (zmk_keymap_layer_active_with_state(layer_id,
                                               zmk_keymap_active_behavior_layer[position])) {
            int ret =
                zmk_keymap_apply_position_state(source, layer_id, position, pressed, timestamp);
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

    for (int layer_idx = ZMK_KEYMAP_LAYERS_LEN - 1; layer_idx >= 0; layer_idx--) {
        uint8_t layer_id = LAYER_INDEX_TO_ID(layer_idx);

        if (layer_id >= ZMK_KEYMAP_LAYERS_LEN) {
            continue;
        }

        struct zmk_behavior_binding *binding = &zmk_sensor_keymap[layer_id][sensor_index];

        LOG_DBG("layer idx: %d, layer id: %d sensor_index: %d, binding name: %s", layer_idx,
                layer_id, sensor_index, binding->behavior_dev);

        const struct device *behavior = zmk_behavior_get_binding(binding->behavior_dev);
        if (!behavior) {
            LOG_DBG("No behavior assigned to %d on layer %d", sensor_index, layer_id);
            continue;
        }

        struct zmk_behavior_binding_event event = {
            .layer = layer_id,
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
            (!opaque_response && layer_idx >= LAYER_ID_TO_INDEX(_zmk_keymap_layer_default) &&
             zmk_keymap_layer_active(layer_id))
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

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)

static int keymap_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;

    LOG_DBG("Setting Keymap setting %s", name);

    if (settings_name_steq(name, "l_n", &next) && next) {
        char *endptr;
        zmk_keymap_layer_id_t layer = strtoul(next, &endptr, 10);

        if (*endptr != '\0') {
            LOG_WRN("Invalid layer number: %s with endptr %s", next, endptr);
            return -EINVAL;
        }

        if (layer >= ZMK_KEYMAP_LAYERS_LEN) {
            LOG_WRN("Found layer name for invalid layer ID %d", layer);
        }

        int ret = read_cb(cb_arg, zmk_keymap_layer_names[layer],
                          MIN(len, CONFIG_ZMK_KEYMAP_LAYER_NAME_MAX_LEN - 1));
        if (ret <= 0) {
            LOG_ERR("Failed to handle keymap layer name from settings (err %d)", ret);
            return ret;
        }

        zmk_keymap_layer_names[layer][ret] = 0;
    } else if (settings_name_steq(name, "l", &next) && next) {
        char *endptr;
        uint8_t layer = strtoul(next, &endptr, 10);
        if (*endptr != '/') {
            LOG_WRN("Invalid layer number: %s with endptr %s", next, endptr);
            return -EINVAL;
        }

        uint8_t key_position = strtoul(endptr + 1, &endptr, 10);

        if (*endptr != '\0') {
            LOG_WRN("Invalid key_position number: %s with endptr %s", next, endptr);
            return -EINVAL;
        }

        if (len > sizeof(struct zmk_behavior_binding_setting)) {
            LOG_ERR("Too large binding setting size (got %d expected %d)", len,
                    sizeof(struct zmk_behavior_binding_setting));
            return -EINVAL;
        }

        if (layer >= ZMK_KEYMAP_LAYERS_LEN) {
            LOG_WRN("Layer %d is larger than max of %d", layer, ZMK_KEYMAP_LAYERS_LEN);
            return -EINVAL;
        }

        if (key_position >= ZMK_KEYMAP_LEN) {
            LOG_WRN("Key position %d is larger than max of %d", key_position, ZMK_KEYMAP_LEN);
            return -EINVAL;
        }

        struct zmk_behavior_binding_setting binding_setting = {0};
        int err = read_cb(cb_arg, &binding_setting, len);
        if (err <= 0) {
            LOG_ERR("Failed to handle keymap binding from settings (err %d)", err);
            return err;
        }

        const char *name =
            zmk_behavior_find_behavior_name_from_local_id(binding_setting.behavior_local_id);

        if (!name) {
            LOG_WRN("Loaded device %d from settings but no device found by that local ID",
                    binding_setting.behavior_local_id);
        }

        zmk_keymap[layer][key_position] = (struct zmk_behavior_binding){
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS_IN_BINDINGS)
            .local_id = binding_setting.behavior_local_id,
#endif
            .behavior_dev = name,
            .param1 = binding_setting.param1,
            .param2 = binding_setting.param2,
        };
    }
#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)
    else if (settings_name_steq(name, "layer_order", &next) && !next) {
        int err =
            read_cb(cb_arg, settings_layer_orders, MIN(len, ARRAY_SIZE(settings_layer_orders)));
        if (err <= 0) {
            LOG_ERR("Failed to handle keymap layer orders from settings (err %d)", err);
            return err;
        }

        LOG_HEXDUMP_DBG(settings_layer_orders, ARRAY_SIZE(settings_layer_orders),
                        "Settings Layer Order");

        memcpy(keymap_layer_orders, settings_layer_orders,
               MIN(len, ARRAY_SIZE(settings_layer_orders)));
    }
#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

    return 0;
};

static int keymap_handle_commit(void) {
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS_IN_BINDINGS)
    for (int l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
        for (int p = 0; p < ZMK_KEYMAP_LEN; p++) {
            struct zmk_behavior_binding *binding = &zmk_keymap[l][p];

            if (binding->local_id > 0 && !binding->behavior_dev) {
                binding->behavior_dev =
                    zmk_behavior_find_behavior_name_from_local_id(binding->local_id);

                if (!binding->behavior_dev) {
                    LOG_ERR("Failed to finding device for local ID %d after settings load",
                            binding->local_id);
                }
            }
        }
    }
#endif

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(keymap, "keymap", NULL, keymap_handle_set, keymap_handle_commit,
                               NULL);

#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)

int keymap_init(void) {
#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)
    load_stock_keymap_layer_ordering();
#endif

    return 0;
}

SYS_INIT(keymap_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
