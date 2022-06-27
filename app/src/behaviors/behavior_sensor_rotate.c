/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <drivers/sensor.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior_queue.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_sensor_rotate_config {
    struct zmk_behavior_binding cw_binding;
    struct zmk_behavior_binding ccw_binding;
    int tap_ms;
};

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
                                       const struct device *sensor,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_rotate_config *cfg = dev->config;

    struct sensor_value value;
    int err;

    err = sensor_channel_get(sensor, SENSOR_CHAN_ROTATION, &value);

    if (err < 0) {
        LOG_WRN("Failed to get sensor rotation value: %d", err);
        return err;
    }

    struct zmk_behavior_binding *triggered_binding;
    switch (value.val1) {
    case 1:
        triggered_binding = (struct zmk_behavior_binding *)&cfg->cw_binding;
        break;
    case -1:
        triggered_binding = (struct zmk_behavior_binding *)&cfg->ccw_binding;
        break;
    default:
        return -ENOTSUP;
    }

    LOG_DBG("Sensor binding: %s", log_strdup(binding->behavior_dev));

    zmk_behavior_queue_add(ZMK_KEYMAP_LEN + event.position, *triggered_binding, true, cfg->tap_ms);
    zmk_behavior_queue_add(ZMK_KEYMAP_LEN + event.position, *triggered_binding, false, 0);

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_sensor_rotate_driver_api = {
    .sensor_binding_triggered = on_sensor_binding_triggered};

static int behavior_sensor_rotate_init(const struct device *dev) { return 0; };

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
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
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(                                                                         \
        n, behavior_sensor_rotate_init, NULL, NULL, &behavior_sensor_rotate_config_##n,            \
        APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_sensor_rotate_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_ROTATE_INST)
