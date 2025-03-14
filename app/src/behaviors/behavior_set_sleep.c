/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_set_sleep

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <dt-bindings/zmk/set_sleep.h>

#include <zmk/behavior.h>
#include <zmk/activity.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata std_values[] = {
    {
        .value = SLEEP_TOGG,
        .display_name = "Toggle Sleep State",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
    },
    {
        .value = SLEEP_ON,
        .display_name = "Enable Sleep",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
    },
    {
        .value = SLEEP_OFF,
        .display_name = "Disable Sleep",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
    },
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
    case SLEEP_TOGG:
        zmk_toggle_sleep();
        return 0;
    case SLEEP_ON:
        zmk_enable_sleep();
        return 0;
    case SLEEP_OFF:
        zmk_disable_sleep();
        return 0;
    default:
        LOG_ERR("Unknown output command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static const struct behavior_driver_api behavior_set_sleep_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_set_sleep_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
