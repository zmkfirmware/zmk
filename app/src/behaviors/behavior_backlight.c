/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_backlight

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <dt-bindings/zmk/backlight.h>
#include <zmk/backlight.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_backlight_init(const struct device *dev) { return 0; }

static int
on_keymap_binding_convert_central_state_dependent_params(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case BL_TOG_CMD:
        binding->param1 = zmk_backlight_is_on() ? BL_OFF_CMD : BL_ON_CMD;
        break;
    case BL_INC_CMD:
        binding->param1 = BL_SET_CMD;
        binding->param2 = zmk_backlight_calc_brt(1);
        break;
    case BL_DEC_CMD:
        binding->param1 = BL_SET_CMD;
        binding->param2 = zmk_backlight_calc_brt(-1);
        break;
    case BL_CYCLE_CMD:
        binding->param1 = BL_SET_CMD;
        binding->param2 = zmk_backlight_calc_brt_cycle();
        break;
    default:
        return 0;
    }

    LOG_DBG("Backlight relative to absolute (%d/%d)", binding->param1, binding->param2);

    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case BL_ON_CMD:
        return zmk_backlight_on();
    case BL_OFF_CMD:
        return zmk_backlight_off();
    case BL_TOG_CMD:
        return zmk_backlight_toggle();
    case BL_INC_CMD: {
        uint8_t brt = zmk_backlight_calc_brt(1);
        return zmk_backlight_set_brt(brt);
    }
    case BL_DEC_CMD: {
        uint8_t brt = zmk_backlight_calc_brt(-1);
        return zmk_backlight_set_brt(brt);
    }
    case BL_CYCLE_CMD: {
        uint8_t brt = zmk_backlight_calc_brt_cycle();
        return zmk_backlight_set_brt(brt);
    }
    case BL_SET_CMD:
        return zmk_backlight_set_brt(binding->param2);
    default:
        LOG_ERR("Unknown backlight command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_backlight_driver_api = {
    .binding_convert_central_state_dependent_params =
        on_keymap_binding_convert_central_state_dependent_params,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

DEVICE_DT_INST_DEFINE(0, behavior_backlight_init, device_pm_control_nop, NULL, NULL, APPLICATION,
                      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_backlight_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
