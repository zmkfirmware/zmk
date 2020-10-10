/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_ext_power

#include <device.h>
#include <devicetree.h>
#include <drivers/behavior.h>

#include <dt-bindings/zmk/ext_power.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/ext_power.h>

static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t command, u32_t arg) {
    const struct device *ext_power = device_get_binding("EXT_POWER");
    if (ext_power == NULL) {
        LOG_ERR("Unable to retrieve ext_power device: %d", command);
        return -EIO;
    }
    const struct ext_power_api *ext_power_api = ext_power->driver_api;

    switch (command) {
    case EXT_POWER_OFF_CMD:
        return ext_power_api->disable(ext_power);
    case EXT_POWER_ON_CMD:
        return ext_power_api->enable(ext_power);
    default:
        LOG_ERR("Unknown ext_power command: %d", command);
    }

    return -ENOTSUP;
}

static int behavior_ext_power_init(struct device *dev) { return 0; };

static int on_keymap_binding_released(struct device *dev, u32_t position, u32_t command,
                                      u32_t arg) {
    return 0;
}

static const struct behavior_driver_api behavior_ext_power_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

DEVICE_AND_API_INIT(behavior_ext_power, DT_INST_LABEL(0), behavior_ext_power_init, NULL, NULL,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_ext_power_driver_api);
