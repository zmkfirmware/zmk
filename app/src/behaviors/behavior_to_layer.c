/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_to_layer

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_to_config {};
struct behavior_to_data {};

static int behavior_to_init(const struct device *dev) { return 0; };

static int to_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return zmk_keymap_layer_to(binding->param1);
}

static int to_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return 0;
}

static const struct behavior_driver_api behavior_to_driver_api = {
    .binding_pressed = to_keymap_binding_pressed,
    .binding_released = to_keymap_binding_released,
};

static const struct behavior_to_config behavior_to_config = {};

static struct behavior_to_data behavior_to_data;

DEVICE_AND_API_INIT(behavior_to, DT_INST_LABEL(0), behavior_to_init, &behavior_to_data,
                    &behavior_to_config, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_to_driver_api);
