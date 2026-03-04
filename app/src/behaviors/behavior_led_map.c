/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_led_map

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <dt-bindings/zmk/led_map.h>
#include <zmk/led_map.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata no_arg_values[] = {
    {
        .display_name = "Toggle On/Off",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = LED_MAP_TOG_CMD,
    },
    {
        .display_name = "Turn On",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = LED_MAP_ON_CMD,
    },
    {
        .display_name = "Turn Off",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = LED_MAP_OFF_CMD,
    },
    {
        .display_name = "Next Effect",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = LED_MAP_EFF_CMD,
    },
    {
        .display_name = "Previous Effect",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = LED_MAP_EFR_CMD,
    },
};

static const struct behavior_parameter_metadata_set no_args_set = {
    .param1_values = no_arg_values,
    .param1_values_len = ARRAY_SIZE(no_arg_values),
};

static const struct behavior_parameter_metadata_set sets[] = {
    no_args_set,
};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(sets),
    .sets = sets,
};

#endif /* IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA) */

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case LED_MAP_TOG_CMD:
        return zmk_led_map_toggle();
    case LED_MAP_ON_CMD:
        return zmk_led_map_on();
    case LED_MAP_OFF_CMD:
        return zmk_led_map_off();
    case LED_MAP_EFF_CMD:
        return zmk_led_map_cycle_effect(1);
    case LED_MAP_EFR_CMD:
        return zmk_led_map_cycle_effect(-1);
    case LED_MAP_EFS_CMD:
        return zmk_led_map_select_effect(binding->param2);
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_led_map_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_led_map_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
