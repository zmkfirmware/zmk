/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_set_layer_binding_at_idx

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_set_layer_binding_at_idx_config {
    size_t bindings_len;
    struct zmk_behavior_binding *bindings;
};

struct behavior_set_layer_binding_at_idx_data {
    size_t current_idx;
};

static int on_set_layer_binding_at_idx_binding_pressed(struct zmk_behavior_binding *binding,
                                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_set_layer_binding_at_idx_config *cfg = dev->config;
    struct behavior_set_layer_binding_at_idx_data *data = dev->data;

    if (cfg->bindings_len == 0) {
        LOG_ERR("No bindings configured");
        return -EINVAL;
    }

    struct zmk_behavior_binding *binding_to_set = &cfg->bindings[data->current_idx];

    int result =
        zmk_keymap_set_layer_binding_at_idx(binding->param1, binding->param2, *binding_to_set);

    if (result < 0) {
        LOG_ERR("Failed to set binding at layer %d, index %d (err: %d)", binding->param1,
                binding->param2, result);
        return result;
    }

    LOG_DBG("Set binding at layer %d, index %d to binding %zu/%zu", binding->param1,
            binding->param2, data->current_idx + 1, cfg->bindings_len);

    // Move to next binding, wrap around if at end
    data->current_idx = (data->current_idx + 1) % cfg->bindings_len;

    return 0;
}

static int on_set_layer_binding_at_idx_binding_released(struct zmk_behavior_binding *binding,
                                                        struct zmk_behavior_binding_event event) {
    return 0;
}

static const struct behavior_driver_api behavior_set_layer_binding_at_idx_driver_api = {
    .binding_pressed = on_set_layer_binding_at_idx_binding_pressed,
    .binding_released = on_set_layer_binding_at_idx_binding_released};

static int behavior_set_layer_binding_at_idx_init(const struct device *dev) {
    struct behavior_set_layer_binding_at_idx_data *data = dev->data;
    data->current_idx = 0;
    return 0;
};

#define _TRANSFORM_ENTRY(idx, node) ZMK_KEYMAP_EXTRACT_BINDING(idx, node)

#define TRANSFORMED_BINDINGS(node)                                                                 \
    {LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_ENTRY, (, ), DT_DRV_INST(node))}

#define SET_LAYER_BINDING_AT_IDX_INST(n)                                                           \
    static struct behavior_set_layer_binding_at_idx_data                                           \
        behavior_set_layer_binding_at_idx_data_##n = {};                                           \
                                                                                                   \
    static const struct zmk_behavior_binding                                                       \
        behavior_set_layer_binding_at_idx_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =   \
            TRANSFORMED_BINDINGS(n);                                                               \
    static const struct behavior_set_layer_binding_at_idx_config                                   \
        behavior_set_layer_binding_at_idx_config_##n = {                                           \
            .bindings_len = DT_INST_PROP_LEN(n, bindings),                                         \
            .bindings = behavior_set_layer_binding_at_idx_config_##n##_bindings};                  \
                                                                                                   \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_set_layer_binding_at_idx_init, NULL,                       \
                            &behavior_set_layer_binding_at_idx_data_##n,                           \
                            &behavior_set_layer_binding_at_idx_config_##n, POST_KERNEL,            \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_set_layer_binding_at_idx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SET_LAYER_BINDING_AT_IDX_INST)

#endif // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
