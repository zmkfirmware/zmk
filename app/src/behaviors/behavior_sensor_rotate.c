/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate

#include <zephyr/device.h>

#include <drivers/behavior.h>

#include "behavior_sensor_rotate_common.h"

static const struct behavior_driver_api behavior_sensor_rotate_driver_api = {
    .sensor_binding_process = zmk_behavior_sensor_rotate_common_process};

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),               \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    }

#define SENSOR_ROTATE_INST(n)                                                                      \
    static struct behavior_sensor_rotate_config behavior_sensor_rotate_config_##n = {              \
        .cw_binding = _TRANSFORM_ENTRY(0, n),                                                      \
        .ccw_binding = _TRANSFORM_ENTRY(1, n),                                                     \
        .tap_ms = DT_INST_PROP_OR(n, tap_ms, 5),                                                   \
        .override_params = false,                                                                  \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_sensor_rotate_config_##n, POST_KERNEL,  \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_sensor_rotate_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_ROTATE_INST)
