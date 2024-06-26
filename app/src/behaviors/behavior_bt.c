/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_bluetooth

#include <zephyr/device.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/behavior.h>
#include <dt-bindings/zmk/bt.h>
#include <zmk/behavior.h>

#include <zmk/ble.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata no_arg_values[] = {
    {
        .display_name = "Next Profile",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BT_NXT_CMD,
    },
    {
        .display_name = "Previous Profile",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BT_PRV_CMD,
    },
    {
        .display_name = "Clear All Profiles",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BT_CLR_ALL_CMD,
    },
    {
        .display_name = "Clear Selected Profile",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BT_CLR_CMD,
    },
};

static const struct behavior_parameter_metadata_set no_args_set = {
    .param1_values = no_arg_values,
    .param1_values_len = ARRAY_SIZE(no_arg_values),
};

static const struct behavior_parameter_value_metadata prof_index_param1_values[] = {
    {
        .display_name = "Select Profile",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BT_SEL_CMD,
    },
    {
        .display_name = "Disconnect Profile",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BT_DISC_CMD,
    },
};

static const struct behavior_parameter_value_metadata prof_index_param2_values[] = {
    {
        .display_name = "Profile",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
        .range = {.min = 0, .max = ZMK_BLE_PROFILE_COUNT},
    },
};

static const struct behavior_parameter_metadata_set profile_index_metadata_set = {
    .param1_values = prof_index_param1_values,
    .param1_values_len = ARRAY_SIZE(prof_index_param1_values),
    .param2_values = prof_index_param2_values,
    .param2_values_len = ARRAY_SIZE(prof_index_param2_values),
};

static const struct behavior_parameter_metadata_set metadata_sets[] = {no_args_set,
                                                                       profile_index_metadata_set};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(metadata_sets),
    .sets = metadata_sets,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case BT_CLR_CMD:
        zmk_ble_clear_bonds();
        return 0;
    case BT_NXT_CMD:
        return zmk_ble_prof_next();
    case BT_PRV_CMD:
        return zmk_ble_prof_prev();
    case BT_SEL_CMD:
        return zmk_ble_prof_select(binding->param2);
    case BT_CLR_ALL_CMD:
        zmk_ble_clear_all_bonds();
        return 0;
    case BT_DISC_CMD:
        return zmk_ble_prof_disconnect(binding->param2);
    default:
        LOG_ERR("Unknown BT command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static int behavior_bt_init(const struct device *dev) { return 0; };

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_bt_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_bt_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_bt_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
