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
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_mo_config {};
struct behavior_mo_data {};

static int behavior_mo_init(const struct device *dev) { return 0; };

static int mo_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return zmk_keymap_layer_activate(binding->param1);
}

static int mo_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return zmk_keymap_layer_deactivate(binding->param1);
}

static const struct behavior_driver_api behavior_mo_driver_api = {
    .binding_pressed = mo_keymap_binding_pressed, .binding_released = mo_keymap_binding_released};

static const struct behavior_mo_config behavior_mo_config = {};

static struct behavior_mo_data behavior_mo_data;

DEVICE_DT_INST_DEFINE(0, behavior_mo_init, device_pm_control_nop, &behavior_mo_data,
                      &behavior_mo_config, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                      &behavior_mo_driver_api);
