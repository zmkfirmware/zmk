/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_none

#include <device.h>
#include <power/reboot.h>
#include <drivers/behavior.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_none_config {};
struct behavior_none_data {};

static int behavior_none_init(struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t _param1,
                                     u32_t _param2, s64_t _timestamp) {
    return 0;
}

static int on_keymap_binding_released(struct device *dev, u32_t position, u32_t _param1,
                                      u32_t _param2, s64_t _timestamp) {
    return 0;
}

static const struct behavior_driver_api behavior_none_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

static const struct behavior_none_config behavior_none_config = {};

static struct behavior_none_data behavior_none_data;

DEVICE_AND_API_INIT(behavior_none, DT_INST_LABEL(0), behavior_none_init, &behavior_none_data,
                    &behavior_none_config, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_none_driver_api);