/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_toggle_layer

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

enum toggle_mode {
    ON,
    OFF,
    FLIP,
};

struct behavior_tog_config {
    enum toggle_mode toggle_mode;
    bool locking;
};

static int tog_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);

    const struct behavior_tog_config *cfg = zmk_behavior_get_binding(binding->behavior_dev)->config;
    switch (cfg->toggle_mode) {
    case ON:
        return zmk_keymap_layer_activate(binding->param1, cfg->locking);
    case OFF:
        return zmk_keymap_layer_deactivate(binding->param1, cfg->locking);
    case FLIP:
        return zmk_keymap_layer_toggle(binding->param1, cfg->locking);
    default:
        return -ENOTSUP;
    };
}

static int tog_keymap_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
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

#endif

static const struct behavior_driver_api behavior_tog_driver_api = {
    .binding_pressed = tog_keymap_binding_pressed,
    .binding_released = tog_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define TG_INST(n)                                                                                 \
    static const struct behavior_tog_config behavior_tog_config_##n = {                            \
        .toggle_mode = DT_ENUM_IDX(DT_DRV_INST(n), toggle_mode),                                   \
        .locking = DT_INST_PROP_OR(n, locking, false),                                             \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_tog_config_##n, POST_KERNEL,            \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_tog_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TG_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
