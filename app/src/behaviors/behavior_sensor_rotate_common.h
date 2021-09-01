/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/behavior.h>
#include <zmk/sensors.h>

struct behavior_sensor_rotate_config {
    struct zmk_behavior_binding cw_binding;
    struct zmk_behavior_binding ccw_binding;
    int tap_ms;
    bool override_params;
};

struct behavior_sensor_rotate_data {
    struct sensor_value remainder[ZMK_KEYMAP_SENSORS_LEN];
};

int zmk_behavior_sensor_rotate_common_trigger(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event,
                                              const struct zmk_sensor_config *sensor_config,
                                              size_t channel_data_size,
                                              const struct zmk_sensor_channel_data *channel_data);