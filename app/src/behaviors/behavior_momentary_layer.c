/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_momentary_layer

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_mo_config {};
struct behavior_mo_data {};

static int behavior_mo_init(struct device *dev) { return 0; };

static int mo_keymap_binding_pressed(struct device *dev, u32_t position, u32_t layer, u32_t _,
                                     s64_t _timestamp) {
    LOG_DBG("position %d layer %d", position, layer);
    return zmk_keymap_layer_activate(layer);
}

static int mo_keymap_binding_released(struct device *dev, u32_t position, u32_t layer, u32_t _,
                                      s64_t _timestamp) {
    LOG_DBG("position %d layer %d", position, layer);
    return zmk_keymap_layer_deactivate(layer);
}

static const struct behavior_driver_api behavior_mo_driver_api = {
    .binding_pressed = mo_keymap_binding_pressed, .binding_released = mo_keymap_binding_released};

static const struct behavior_mo_config behavior_mo_config = {};

static struct behavior_mo_data behavior_mo_data;

DEVICE_AND_API_INIT(behavior_mo, DT_INST_LABEL(0), behavior_mo_init, &behavior_mo_data,
                    &behavior_mo_config, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_mo_driver_api);
