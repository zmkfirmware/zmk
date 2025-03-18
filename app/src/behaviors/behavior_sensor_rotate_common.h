/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <zmk/behavior.h>

struct behavior_sensor_rotate_config {
    struct zmk_behavior_binding cw_binding;
    struct zmk_behavior_binding ccw_binding;
    int tap_ms;
    bool override_params;
};

int zmk_behavior_sensor_rotate_common_process(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event);