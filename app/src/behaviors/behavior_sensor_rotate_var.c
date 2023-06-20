/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate_var

#include <zephyr/device.h>

#include <drivers/behavior.h>

#include "behavior_sensor_rotate_common.h"

static const struct behavior_driver_api behavior_sensor_rotate_var_driver_api = {
    .sensor_binding_accept_data = zmk_behavior_sensor_rotate_common_accept_data,
    .sensor_binding_process = zmk_behavior_sensor_rotate_common_process};

static int behavior_sensor_rotate_var_init(const struct device *dev) { return 0; };

#define SENSOR_ROTATE_VAR_INST(n)                                                                  \
    static struct behavior_sensor_rotate_config behavior_sensor_rotate_var_config_##n = {          \
        .cw_binding = {.behavior_dev = DT_PROP(DT_INST_PHANDLE_BY_IDX(n, bindings, 0), label)},    \
        .ccw_binding = {.behavior_dev = DT_PROP(DT_INST_PHANDLE_BY_IDX(n, bindings, 1), label)},   \
        .tap_ms = DT_INST_PROP(n, tap_ms),                                                         \
        .override_params = true,                                                                   \
    };                                                                                             \
    static struct behavior_sensor_rotate_data behavior_sensor_rotate_var_data_##n = {};            \
    DEVICE_DT_INST_DEFINE(                                                                         \
        n, behavior_sensor_rotate_var_init, NULL, &behavior_sensor_rotate_var_data_##n,            \
        &behavior_sensor_rotate_var_config_##n, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,  \
        &behavior_sensor_rotate_var_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_ROTATE_VAR_INST)
