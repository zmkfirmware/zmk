/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_outputs

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <drivers/behavior.h>

#include <dt-bindings/zmk/outputs.h>

#include <zmk/behavior.h>
#include <zmk/endpoints.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata std_values[] = {
    {
        .value = OUT_TOG,
        .display_name = "Toggle Outputs",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
    },
#if IS_ENABLED(CONFIG_ZMK_USB)
    {
        .value = OUT_USB,
        .display_name = "USB Output",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
    },
#endif // IS_ENABLED(CONFIG_ZMK_USB)
#if IS_ENABLED(CONFIG_ZMK_BLE)
    {
        .value = OUT_BLE,
        .display_name = "BLE Output",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
    },
#endif // IS_ENABLED(CONFIG_ZMK_BLE)
};

static const struct behavior_parameter_metadata_set std_set = {
    .param1_values = std_values,
    .param1_values_len = ARRAY_SIZE(std_values),
};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = 1,
    .sets = &std_set,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case OUT_TOG:
        return zmk_endpoints_toggle_transport();
    case OUT_USB:
        return zmk_endpoints_select_transport(ZMK_TRANSPORT_USB);
    case OUT_BLE:
        return zmk_endpoints_select_transport(ZMK_TRANSPORT_BLE);
    default:
        LOG_ERR("Unknown output command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static const struct behavior_driver_api behavior_outputs_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_outputs_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
