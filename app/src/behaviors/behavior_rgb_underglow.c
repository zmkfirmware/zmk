/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_rgb_underglow

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <dt-bindings/zmk/rgb.h>
#include <zmk/rgb_underglow.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata no_arg_values[] = {
    {
        .display_name = "Toggle On/Off",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_TOG_CMD,
    },
    {
        .display_name = "Turn On",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_ON_CMD,
    },
    {
        .display_name = "Turn OFF",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_OFF_CMD,
    },
    {
        .display_name = "Hue Up",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_HUI_CMD,
    },
    {
        .display_name = "Hue Down",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_HUD_CMD,
    },
    {
        .display_name = "Saturation Up",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_SAI_CMD,
    },
    {
        .display_name = "Saturation Down",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_SAD_CMD,
    },
    {
        .display_name = "Brightness Up",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_BRI_CMD,
    },
    {
        .display_name = "Brightness Down",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_BRD_CMD,
    },
    {
        .display_name = "Speed Up",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_SPI_CMD,
    },
    {
        .display_name = "Speed Down",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_SPD_CMD,
    },
    {
        .display_name = "Next Effect",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_EFF_CMD,
    },
    {
        .display_name = "Previous Effect",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_EFR_CMD,
    },
};

static const struct behavior_parameter_metadata_set no_args_set = {
    .param1_values = no_arg_values,
    .param1_values_len = ARRAY_SIZE(no_arg_values),
};

/*
static const struct behavior_parameter_value_metadata hsv_p1_value_metadata_values[] = {
    {
        .display_name = "Set Color",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = RGB_COLOR_HSB_CMD,
    },
};

static const struct behavior_parameter_value_metadata hsv_p2_value_metadata_values[] = {
    {
        .display_name = "Color",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_STANDARD,
        .standard = BEHAVIOR_PARAMETER_STANDARD_DOMAIN_HSV,
    },
};

static const struct behavior_parameter_metadata_set hsv_value_metadata_set = {
    .param1_values = hsv_p1_value_metadata_values,
    .param1_values_len = ARRAY_SIZE(hsv_p1_value_metadata_values),
    .param_values = hsv_p2_value_metadata_values,
    .param_values_len = ARRAY_SIZE(hsv_p2_value_metadata_values),
};

*/

static const struct behavior_parameter_metadata_set sets[] = {
    no_args_set,
    // hsv_value_metadata_set,
};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(sets),
    .sets = sets,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int behavior_rgb_underglow_init(const struct device *dev) { return 0; }

static int
on_keymap_binding_convert_central_state_dependent_params(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case RGB_TOG_CMD: {
        bool state;
        int err = zmk_rgb_underglow_get_state(&state);
        if (err) {
            LOG_ERR("Failed to get RGB underglow state (err %d)", err);
            return err;
        }

        binding->param1 = state ? RGB_OFF_CMD : RGB_ON_CMD;
        break;
    }
    case RGB_BRI_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_brt(1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_BRD_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_brt(-1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_HUI_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_HUD_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(-1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_SAI_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_sat(1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_SAD_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_sat(-1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_EFR_CMD: {
        binding->param1 = RGB_EFS_CMD;
        binding->param2 = zmk_rgb_underglow_calc_effect(-1);
        break;
    }
    case RGB_EFF_CMD: {
        binding->param1 = RGB_EFS_CMD;
        binding->param2 = zmk_rgb_underglow_calc_effect(1);
        break;
    }
    default:
        return 0;
    }

    LOG_DBG("RGB relative convert to absolute (%d/%d)", binding->param1, binding->param2);

    return 0;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case RGB_TOG_CMD:
        return zmk_rgb_underglow_toggle();
    case RGB_ON_CMD:
        return zmk_rgb_underglow_on();
    case RGB_OFF_CMD:
        return zmk_rgb_underglow_off();
    case RGB_HUI_CMD:
        return zmk_rgb_underglow_change_hue(1);
    case RGB_HUD_CMD:
        return zmk_rgb_underglow_change_hue(-1);
    case RGB_SAI_CMD:
        return zmk_rgb_underglow_change_sat(1);
    case RGB_SAD_CMD:
        return zmk_rgb_underglow_change_sat(-1);
    case RGB_BRI_CMD:
        return zmk_rgb_underglow_change_brt(1);
    case RGB_BRD_CMD:
        return zmk_rgb_underglow_change_brt(-1);
    case RGB_SPI_CMD:
        return zmk_rgb_underglow_change_spd(1);
    case RGB_SPD_CMD:
        return zmk_rgb_underglow_change_spd(-1);
    case RGB_EFS_CMD:
        return zmk_rgb_underglow_select_effect(binding->param2);
    case RGB_EFF_CMD:
        return zmk_rgb_underglow_cycle_effect(1);
    case RGB_EFR_CMD:
        return zmk_rgb_underglow_cycle_effect(-1);
    case RGB_COLOR_HSB_CMD:
        return zmk_rgb_underglow_set_hsb((struct zmk_led_hsb){.h = (binding->param2 >> 16) & 0xFFFF,
                                                              .s = (binding->param2 >> 8) & 0xFF,
                                                              .b = binding->param2 & 0xFF});
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_rgb_underglow_driver_api = {
    .binding_convert_central_state_dependent_params =
        on_keymap_binding_convert_central_state_dependent_params,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_rgb_underglow_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_rgb_underglow_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
