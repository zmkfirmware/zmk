/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/mouse/input_config.h>
#include <zephyr/device.h>

#define DT_DRV_COMPAT zmk_input_configs

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define CHILD_CONFIG(inst)                                                                         \
    {                                                                                              \
        .dev = DEVICE_DT_GET(DT_PHANDLE(inst, device)),                                            \
        .xy_swap = DT_PROP(inst, xy_swap),                                                         \
        .x_invert = DT_PROP(inst, x_invert),                                                       \
        .y_invert = DT_PROP(inst, y_invert),                                                       \
        .scale_multiplier = DT_PROP(inst, scale_multiplier),                                       \
        .scale_divisor = DT_PROP(inst, scale_divisor),                                             \
    },

const struct zmk_input_config configs[] = {DT_INST_FOREACH_CHILD(0, CHILD_CONFIG)};

const struct zmk_input_config *zmk_input_config_get_for_device(const struct device *dev) {
    for (int i = 0; i < ARRAY_SIZE(configs); i++) {
        if (configs[i].dev == dev) {
            return &configs[i];
        }
    }

    return NULL;
}

#else

const struct zmk_input_config *zmk_input_config_get_for_device(const struct device *dev) {
    return NULL;
}

#endif