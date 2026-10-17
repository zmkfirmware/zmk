/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_led_matrix

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>

#include <dt-bindings/zmk/led_matrix.h>
#include <zmk/keymap.h>
#include <zmk/led_matrix.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if DT_HAS_CHOSEN(zmk_led_matrix)
#define LED_MATRIX_NODE DT_CHOSEN(zmk_led_matrix)
#define LED_MATRIX_PROFILE_CHILD_COUNT(node_id)                                                   \
    +COND_CODE_1(DT_NODE_HAS_PROP(node_id, profile_id), (1), (0))
#define LED_MATRIX_INDEPENDENT_CHILD_COUNT(node_id)                                               \
    +COND_CODE_1(DT_NODE_HAS_PROP(node_id, independent_id), (1), (0))
#define ZMK_LED_MATRIX_PROFILE_COUNT                                                              \
    (0 DT_FOREACH_CHILD(LED_MATRIX_NODE, LED_MATRIX_PROFILE_CHILD_COUNT))
#define ZMK_LED_MATRIX_INDEPENDENT_COUNT                                                          \
    (0 DT_FOREACH_CHILD(LED_MATRIX_NODE, LED_MATRIX_INDEPENDENT_CHILD_COUNT))
#else
#define ZMK_LED_MATRIX_PROFILE_COUNT 1
#define ZMK_LED_MATRIX_INDEPENDENT_COUNT 0
#endif

#define ZMK_LED_MATRIX_PROFILE_MAX                                                                \
    ((ZMK_LED_MATRIX_PROFILE_COUNT > 0) ? (ZMK_LED_MATRIX_PROFILE_COUNT - 1) : 0)
#define ZMK_LED_MATRIX_INDEPENDENT_MAX                                                            \
    ((ZMK_LED_MATRIX_INDEPENDENT_COUNT > 0) ? (ZMK_LED_MATRIX_INDEPENDENT_COUNT - 1) : 0)
#define ZMK_LED_MATRIX_SPEED_MAX CONFIG_ZMK_LED_MATRIX_SPD_LEVELS
#define ZMK_LED_MATRIX_HUE_INDEX_MAX 11

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata no_arg_values[] = {
    {.display_name = "Turn On", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = LM_ON_CMD},
    {.display_name = "Turn Off", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = LM_OFF_CMD},
    {.display_name = "Toggle", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = LM_TOG_CMD},
    {.display_name = "Brightness Up",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_BRI_CMD},
    {.display_name = "Brightness Down",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_BRD_CMD},
    {.display_name = "Profile Next",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_EFF_CMD},
    {.display_name = "Profile Previous",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_EFR_CMD},
    {.display_name = "Speed Up",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_SPI_CMD},
    {.display_name = "Speed Down",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_SPD_CMD},
    {.display_name = "Hue Up", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = LM_HUI_CMD},
    {.display_name = "Hue Down", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = LM_HUD_CMD},
};

static const struct behavior_parameter_value_metadata profile_p1_values[] = {
    {.display_name = "Set Profile",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_EFS_CMD},
};

static const struct behavior_parameter_value_metadata profile_p2_values[] = {
    {.display_name = "Profile",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
     .range =
         {
             .min = 0,
             .max = ZMK_LED_MATRIX_PROFILE_MAX,
         }},
};

static const struct behavior_parameter_value_metadata brightness_p1_values[] = {
    {.display_name = "Set Brightness",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_BRT_CMD},
};

static const struct behavior_parameter_value_metadata brightness_p2_values[] = {
    {.display_name = "Value",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
     .range =
         {
             .min = 0,
             .max = 100,
         }},
};

static const struct behavior_parameter_value_metadata status_p1_values[] = {
    {.display_name = "Show Status",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_STA_CMD},
};

static const struct behavior_parameter_value_metadata status_p2_values[] = {
    {.display_name = "Mask",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
     .range =
         {
             .min = 0,
             .max = LM_STA_ALL,
         }},
};

static const struct behavior_parameter_value_metadata speed_p1_values[] = {
    {.display_name = "Set Speed",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_SPS_CMD},
};

static const struct behavior_parameter_value_metadata speed_p2_values[] = {
    {.display_name = "Speed",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
     .range =
         {
             .min = 1,
             .max = ZMK_LED_MATRIX_SPEED_MAX,
         }},
};

static const struct behavior_parameter_value_metadata hue_p1_values[] = {
    {.display_name = "Set Palette Hue",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_HUS_CMD},
};

static const struct behavior_parameter_value_metadata hue_p2_values[] = {
    {.display_name = "Hue Index",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
     .range =
         {
             .min = 0,
             .max = ZMK_LED_MATRIX_HUE_INDEX_MAX,
         }},
};

static const struct behavior_parameter_value_metadata child_relative_p1_values[] = {
    {.display_name = "Child Profile Next",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_CFF_CMD},
    {.display_name = "Child Profile Previous",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_CFR_CMD},
};

static const struct behavior_parameter_value_metadata child_relative_p2_values[] = {
    {.display_name = "Child",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
     .range =
         {
             .min = 0,
             .max = ZMK_LED_MATRIX_INDEPENDENT_MAX,
         }},
};

static const struct behavior_parameter_value_metadata child_set_p1_values[] = {
    {.display_name = "Set Child Profile",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
     .value = LM_CFS_CMD},
};

static const struct behavior_parameter_metadata_set no_args_set = {
    .param1_values = no_arg_values,
    .param1_values_len = ARRAY_SIZE(no_arg_values),
};

static const struct behavior_parameter_metadata_set profile_args_set = {
    .param1_values = profile_p1_values,
    .param1_values_len = ARRAY_SIZE(profile_p1_values),
    .param2_values = profile_p2_values,
    .param2_values_len = ARRAY_SIZE(profile_p2_values),
};

static const struct behavior_parameter_metadata_set brightness_args_set = {
    .param1_values = brightness_p1_values,
    .param1_values_len = ARRAY_SIZE(brightness_p1_values),
    .param2_values = brightness_p2_values,
    .param2_values_len = ARRAY_SIZE(brightness_p2_values),
};

static const struct behavior_parameter_metadata_set status_args_set = {
    .param1_values = status_p1_values,
    .param1_values_len = ARRAY_SIZE(status_p1_values),
    .param2_values = status_p2_values,
    .param2_values_len = ARRAY_SIZE(status_p2_values),
};

static const struct behavior_parameter_metadata_set speed_args_set = {
    .param1_values = speed_p1_values,
    .param1_values_len = ARRAY_SIZE(speed_p1_values),
    .param2_values = speed_p2_values,
    .param2_values_len = ARRAY_SIZE(speed_p2_values),
};

static const struct behavior_parameter_metadata_set hue_args_set = {
    .param1_values = hue_p1_values,
    .param1_values_len = ARRAY_SIZE(hue_p1_values),
    .param2_values = hue_p2_values,
    .param2_values_len = ARRAY_SIZE(hue_p2_values),
};

static const struct behavior_parameter_metadata_set child_relative_args_set = {
    .param1_values = child_relative_p1_values,
    .param1_values_len = ARRAY_SIZE(child_relative_p1_values),
    .param2_values = child_relative_p2_values,
    .param2_values_len = ARRAY_SIZE(child_relative_p2_values),
};

static const struct behavior_parameter_metadata_set child_set_args_set = {
    .param1_values = child_set_p1_values,
    .param1_values_len = ARRAY_SIZE(child_set_p1_values),
};

static const struct behavior_parameter_metadata_set sets[] = {
    no_args_set,
    profile_args_set,
    brightness_args_set,
    speed_args_set,
    hue_args_set,
    status_args_set,
    child_relative_args_set,
    child_set_args_set,
};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(sets),
    .sets = sets,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int
on_keymap_binding_convert_central_state_dependent_params(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case LM_TOG_CMD:
        binding->param1 = zmk_led_matrix_is_on() ? LM_OFF_CMD : LM_ON_CMD;
        break;
    case LM_BRI_CMD:
        binding->param1 = LM_BRT_CMD;
        binding->param2 = zmk_led_matrix_calc_brt(1);
        break;
    case LM_BRD_CMD:
        binding->param1 = LM_BRT_CMD;
        binding->param2 = zmk_led_matrix_calc_brt(-1);
        break;
    case LM_EFF_CMD:
        binding->param1 = LM_EFS_CMD;
        binding->param2 = zmk_led_matrix_calc_profile(1);
        break;
    case LM_EFR_CMD:
        binding->param1 = LM_EFS_CMD;
        binding->param2 = zmk_led_matrix_calc_profile(-1);
        break;
    case LM_SPI_CMD:
        binding->param1 = LM_SPS_CMD;
        binding->param2 = zmk_led_matrix_calc_spd(1);
        break;
    case LM_SPD_CMD:
        binding->param1 = LM_SPS_CMD;
        binding->param2 = zmk_led_matrix_calc_spd(-1);
        break;
    case LM_HUI_CMD:
        binding->param1 = LM_HUS_CMD;
        binding->param2 = zmk_led_matrix_calc_hue_index(1);
        break;
    case LM_HUD_CMD:
        binding->param1 = LM_HUS_CMD;
        binding->param2 = zmk_led_matrix_calc_hue_index(-1);
        break;
    case LM_CFF_CMD:
        binding->param1 = LM_CFS_CMD;
        binding->param2 = LM_CHILD_PROFILE_PACK(
            binding->param2, zmk_led_matrix_calc_independent_profile((uint8_t)binding->param2, 1));
        break;
    case LM_CFR_CMD:
        binding->param1 = LM_CFS_CMD;
        binding->param2 = LM_CHILD_PROFILE_PACK(
            binding->param2, zmk_led_matrix_calc_independent_profile((uint8_t)binding->param2, -1));
        break;
    default:
        return 0;
    }

    LOG_DBG("LED matrix relative to absolute (%d/%d)", binding->param1, binding->param2);
    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case LM_ON_CMD:
        return zmk_led_matrix_on();
    case LM_OFF_CMD:
        return zmk_led_matrix_off();
    case LM_TOG_CMD:
        return zmk_led_matrix_toggle();
    case LM_BRI_CMD:
        return zmk_led_matrix_set_brt(zmk_led_matrix_calc_brt(1));
    case LM_BRD_CMD:
        return zmk_led_matrix_set_brt(zmk_led_matrix_calc_brt(-1));
    case LM_BRT_CMD:
        return zmk_led_matrix_set_brt(binding->param2);
    case LM_EFF_CMD:
        return zmk_led_matrix_set_profile(zmk_led_matrix_calc_profile(1));
    case LM_EFR_CMD:
        return zmk_led_matrix_set_profile(zmk_led_matrix_calc_profile(-1));
    case LM_EFS_CMD:
        return zmk_led_matrix_set_profile(binding->param2);
    case LM_SPI_CMD:
        return zmk_led_matrix_set_spd(zmk_led_matrix_calc_spd(1));
    case LM_SPD_CMD:
        return zmk_led_matrix_set_spd(zmk_led_matrix_calc_spd(-1));
    case LM_SPS_CMD:
        return zmk_led_matrix_set_spd(binding->param2);
    case LM_HUI_CMD:
        return zmk_led_matrix_set_hue_index(zmk_led_matrix_calc_hue_index(1));
    case LM_HUD_CMD:
        return zmk_led_matrix_set_hue_index(zmk_led_matrix_calc_hue_index(-1));
    case LM_HUS_CMD:
        return zmk_led_matrix_set_hue_index(binding->param2);
    case LM_CFF_CMD:
        return zmk_led_matrix_set_independent_profile(
            (uint8_t)binding->param2,
            zmk_led_matrix_calc_independent_profile((uint8_t)binding->param2, 1));
    case LM_CFR_CMD:
        return zmk_led_matrix_set_independent_profile(
            (uint8_t)binding->param2,
            zmk_led_matrix_calc_independent_profile((uint8_t)binding->param2, -1));
    case LM_CFS_CMD:
        return zmk_led_matrix_set_independent_profile(
            (uint8_t)LM_CHILD_PROFILE_CHILD_ID(binding->param2),
            (uint8_t)LM_CHILD_PROFILE_PROFILE_ID(binding->param2));
    case LM_STA_CMD:
        return zmk_led_matrix_show_status((uint8_t)binding->param2);
    default:
        LOG_ERR("Unknown LED matrix command: %d", binding->param1);
        return -ENOTSUP;
    }
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_led_matrix_driver_api = {
    .binding_convert_central_state_dependent_params =
        on_keymap_binding_convert_central_state_dependent_params,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_led_matrix_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
