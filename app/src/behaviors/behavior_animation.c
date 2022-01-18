/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/animation/animation_control.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_animation_control

#define DEVICE_ADDR(idx) DEVICE_DT_GET(DT_INST(idx, zmk_animation_control)),

/**
 * Control animation instance pointers.
 */
static const struct device *control_animations[] = {DT_INST_FOREACH_STATUS_OKAY(DEVICE_ADDR)};

/**
 * Size of control animation instance pointers array.
 */
static const uint8_t control_animations_size = sizeof(control_animations);

/**
 * Index of the current default control animation instance.
 */
static uint8_t current_zone = 0;

#define DT_DRV_COMPAT zmk_behavior_animation

static int
on_keymap_binding_convert_central_state_dependent_params(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    if ((binding->param1 >> 24) == 0xff) {
        binding->param1 = (current_zone << 24) | (binding->param1 & 0x00ffffff);
    }

    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    uint8_t value = binding->param1 & 0xff;
    uint8_t command = (binding->param1 >> 16) & 0xff;
    uint8_t zone = (binding->param1 >> 24) & 0xff;

    if (command == ANIMATION_CMD_NEXT_CONTROL_ZONE) {
        current_zone++;

        if (current_zone == control_animations_size) {
            current_zone = 0;
        }
        return 0;
    }

    if (command == ANIMATION_CMD_PREVIOUS_CONTROL_ZONE) {
        if (current_zone == 0) {
            current_zone = control_animations_size;
        }

        current_zone--;
        return 0;
    }

    if (control_animations_size <= zone) {
        return -ENOTSUP;
    }

    return animation_control_handle_command(control_animations[zone], command, value);
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_animation_init(const struct device *dev) { return 0; }

static const struct behavior_driver_api behavior_animation_driver_api = {
    .binding_convert_central_state_dependent_params =
        on_keymap_binding_convert_central_state_dependent_params,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

DEVICE_DT_INST_DEFINE(0, behavior_animation_init, device_pm_control_nop, NULL, NULL, APPLICATION,
                      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_animation_driver_api);
