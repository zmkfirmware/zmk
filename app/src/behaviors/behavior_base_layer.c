/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_base_layer

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#if IS_ENABLED(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct base_layer_state {
    uint8_t layer_by_endpoint[ZMK_ENDPOINT_COUNT];
};

static struct base_layer_state state;

struct behavior_base_layer_config {
    uint8_t base_layers_count;
    uint8_t base_layers[];
};

#if IS_ENABLED(CONFIG_SETTINGS)
static int base_layer_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                   void *cb_arg) {
    const char *next;
    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(state)) {
            return -EINVAL;
        }
        const int err = read_cb(cb_arg, &state, sizeof(state));
        if (err <= 0) {
            LOG_ERR("Failed to read base_layer/state from settings (err %d)", err);
            return err;
        }
    }
    return 0;
}

static void base_layer_save_work_handler(struct k_work *work) {
    settings_save_one("base_layer/state", &state, sizeof(state));
}

static struct k_work_delayable base_layer_save_work;
struct settings_handler base_layer_settings_handler = {.name = "base_layer",
                                                       .h_set = base_layer_settings_set};

static int base_layer_settings_init(void) {
    settings_subsys_init();

    const int err = settings_register(&base_layer_settings_handler);
    if (err) {
        LOG_ERR("Failed to register the base_layer settings handler (err %d)", err);
        return err;
    }

    k_work_init_delayable(&base_layer_save_work, base_layer_save_work_handler);

    return settings_load_subtree("base_layer");
}
SYS_INIT(base_layer_settings_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif // IS_ENABLED(CONFIG_SETTINGS)

static void set_base_layer(uint8_t layer, const struct behavior_base_layer_config *config) {
    const uint8_t n = config->base_layers_count;
    if (n > 0) {
        LOG_DBG("deactivating %d base layers before using zmk_keymap_layer_activate(%d)", n, layer);
        for (uint8_t i = 0; i < n; ++i)
            zmk_keymap_layer_deactivate(config->base_layers[i]);
        zmk_keymap_layer_activate(layer);
    } else {
        LOG_DBG("no base layers set, using zmk_keymap_layer_to(%d)", layer);
        zmk_keymap_layer_to(layer);
    }
}

static int behavior_base_layer_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);

    const struct zmk_endpoint_instance endpoint = zmk_endpoints_selected();
    const int endpoint_index = zmk_endpoint_instance_to_index(endpoint);
    const uint8_t layer = binding->param1;

    state.layer_by_endpoint[endpoint_index] = layer;
    set_base_layer(layer, zmk_behavior_get_binding(binding->behavior_dev)->config);

    char endpoint_str[ZMK_ENDPOINT_STR_LEN];
    zmk_endpoint_instance_to_str(endpoint, endpoint_str, sizeof(endpoint_str));
    LOG_INF("saved base layer %d for endpoint %s", layer, endpoint_str);

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_reschedule(&base_layer_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#endif // IS_ENABLED(CONFIG_SETTINGS)

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int base_layer_listener(const zmk_event_t *e,
                               const struct behavior_base_layer_config *config) {
    struct zmk_endpoint_changed *data = as_zmk_endpoint_changed(e);
    if (data != NULL) {
        const int endpoint_index = zmk_endpoint_instance_to_index(data->endpoint);
        const uint8_t layer = state.layer_by_endpoint[endpoint_index];
        set_base_layer(layer, config);

        char endpoint_str[ZMK_ENDPOINT_STR_LEN];
        zmk_endpoint_instance_to_str(data->endpoint, endpoint_str, sizeof(endpoint_str));
        LOG_INF("restored base layer %d for endpoint %s", layer, endpoint_str);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .display_name = "Layer",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_LAYER_ID,
    },
};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = param_values,
    .param1_values_len = ARRAY_SIZE(param_values),
}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_driver_api behavior_base_layer_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define BASE_LAYERS_ITEM(i, n) (DT_INST_PROP_BY_IDX(n, base_layers, i))

#define BASE_LAYER_INST(n)                                                                         \
    static struct behavior_base_layer_config behavior_base_layer_config_##n = {                    \
        .base_layers = {LISTIFY(DT_INST_PROP_LEN(n, base_layers), BASE_LAYERS_ITEM, (, ), n)},     \
        .base_layers_count = DT_INST_PROP_LEN(n, base_layers),                                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_base_layer_init, NULL, NULL,                               \
                            &behavior_base_layer_config_##n, POST_KERNEL,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_base_layer_driver_api); \
    static int base_layer_listener_##n(const zmk_event_t *e) {                                     \
        return base_layer_listener(e, &behavior_base_layer_config_##n);                            \
    }                                                                                              \
    static ZMK_LISTENER(base_layer_listener_##n, base_layer_listener_##n);                         \
    ZMK_SUBSCRIPTION(base_layer_listener_##n, zmk_endpoint_changed);

DT_INST_FOREACH_STATUS_OKAY(BASE_LAYER_INST)

#endif // DT_HAS_COMPAT_STATUS_OKAY
