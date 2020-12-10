/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_rgb_underglow

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <dt-bindings/zmk/rgb.h>
#include <zmk/rgb_underglow.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int behavior_rgb_underglow_init(const struct device *dev) { return 0; }

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case RGB_TOG:
        return zmk_rgb_underglow_toggle();
    case RGB_HUI:
        return zmk_rgb_underglow_change_hue(1);
    case RGB_HUD:
        return zmk_rgb_underglow_change_hue(-1);
    case RGB_SAI:
        return zmk_rgb_underglow_change_sat(1);
    case RGB_SAD:
        return zmk_rgb_underglow_change_sat(-1);
    case RGB_BRI:
        return zmk_rgb_underglow_change_brt(1);
    case RGB_BRD:
        return zmk_rgb_underglow_change_brt(-1);
    case RGB_SPI:
        return zmk_rgb_underglow_change_spd(1);
    case RGB_SPD:
        return zmk_rgb_underglow_change_spd(-1);
    case RGB_EFF:
        return zmk_rgb_underglow_cycle_effect(1);
    case RGB_EFR:
        return zmk_rgb_underglow_cycle_effect(-1);
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return 0;
}

static const struct behavior_driver_api behavior_rgb_underglow_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

DEVICE_AND_API_INIT(behavior_rgb_underglow, DT_INST_LABEL(0), behavior_rgb_underglow_init, NULL,
                    NULL, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_rgb_underglow_driver_api);