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

static int behavior_to_init(const struct device *dev) { return 0; };

static int to_keymap_binding_pressed(const struct behavior_state_changed *event) {
    LOG_DBG("position %d layer %d", event->position, event->param1);
    zmk_keymap_layer_to(event->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int to_keymap_binding_released(const struct behavior_state_changed *event) {
    LOG_DBG("position %d layer %d", event->position, event->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_to_driver_api = {
    .binding_pressed = to_keymap_binding_pressed,
    .binding_released = to_keymap_binding_released,
};

DEVICE_AND_API_INIT(behavior_to, DT_INST_LABEL(0), behavior_to_init, NULL, NULL, APPLICATION,
                    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_to_driver_api);
