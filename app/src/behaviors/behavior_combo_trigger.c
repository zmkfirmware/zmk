/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_combo_trigger

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/keymap.h>
#include <zmk/combos.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_combo_trigger_config {
    char *fallback_behavior_dev;
};

static int on_combo_trigger_binding_pressed(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    const struct behavior_combo_trigger_config *cfg =
        zmk_behavior_get_binding(binding->behavior_dev)->config;
    return zmk_combo_trigger_behavior_invoked(binding->param1, cfg->fallback_behavior_dev,
                                              binding->param2, event, true);
}

static int on_combo_trigger_binding_released(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    const struct behavior_combo_trigger_config *cfg =
        zmk_behavior_get_binding(binding->behavior_dev)->config;
    return zmk_combo_trigger_behavior_invoked(binding->param1, cfg->fallback_behavior_dev,
                                              binding->param2, event, false);
}

static const struct behavior_driver_api behavior_combo_trigger_driver_api = {
    .binding_pressed = on_combo_trigger_binding_pressed,
    .binding_released = on_combo_trigger_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

static int behavior_combo_trigger_init(const struct device *dev) { return 0; }

#define CT_INST(n)                                                                                 \
    static struct behavior_combo_trigger_config behavior_combo_trigger_config_##n = {              \
        .fallback_behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE(n, fallback_behavior)),            \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(                                                                       \
        n, behavior_combo_trigger_init, NULL, NULL, &behavior_combo_trigger_config_##n,            \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_combo_trigger_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CT_INST)

#endif
