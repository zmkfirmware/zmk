/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_array

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_array_config {
    size_t behavior_count;
    struct zmk_behavior_binding *behaviors;
};

static int on_array_binding_pressed(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_array_config *cfg = dev->config;
    int index = binding->param1;

    if (index >= cfg->behavior_count || index < 0) {
        LOG_ERR("Trying to trigger an index beyond the size of the behavior array.");
        return -ENOTSUP;
    }
    return zmk_behavior_invoke_binding((struct zmk_behavior_binding *)&cfg->behaviors[index], event,
                                       true);
}

static int on_array_binding_released(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_array_config *cfg = dev->config;
    int index = binding->param1;

    if (index >= cfg->behavior_count || index < 0) {
        LOG_ERR("Trying to trigger an index beyond the size of the behavior array.");
        return -ENOTSUP;
    }
    return zmk_behavior_invoke_binding((struct zmk_behavior_binding *)&cfg->behaviors[index], event,
                                       false);
}

static const struct behavior_driver_api behavior_array_driver_api = {
    .binding_pressed = on_array_binding_pressed,
    .binding_released = on_array_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

static int behavior_array_init(const struct device *dev) { return 0; }

#define _TRANSFORM_ENTRY(idx, node) ZMK_KEYMAP_EXTRACT_BINDING(idx, node)

#define TRANSFORMED_BINDINGS(node)                                                                 \
    {LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_ENTRY, (, ), DT_DRV_INST(node))}

#define ARR_INST(n)                                                                                \
    static struct zmk_behavior_binding                                                             \
        behavior_array_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =                      \
            TRANSFORMED_BINDINGS(n);                                                               \
    static struct behavior_array_config behavior_array_config_##n = {                              \
        .behaviors = behavior_array_config_##n##_bindings,                                         \
        .behavior_count = DT_INST_PROP_LEN(n, bindings)};                                          \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_array_init, NULL, NULL, &behavior_array_config_##n,        \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_array_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ARR_INST)

#endif
