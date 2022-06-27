/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate_var

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

struct behavior_sensor_rotate_var_config {
    char *cw_behavior_dev;
    char *ccw_behavior_dev;
    int tap_ms;
};

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
                                       const struct device *sensor,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_rotate_var_config *cfg = dev->config;

    struct sensor_value value;
    int err;

    err = sensor_channel_get(sensor, SENSOR_CHAN_ROTATION, &value);

    if (err < 0) {
        LOG_WRN("Failed to get sensor rotation value: %d", err);
        return err;
    }

    struct zmk_behavior_binding triggered_binding;
    switch (value.val1) {
    case 1:
        triggered_binding.behavior_dev = cfg->cw_behavior_dev;
        triggered_binding.param1 = binding->param1;
        break;
    case -1:
        triggered_binding.behavior_dev = cfg->ccw_behavior_dev;
        triggered_binding.param1 = binding->param2;
        break;
    default:
        return -ENOTSUP;
    }

    LOG_DBG("Sensor binding: %s", log_strdup(binding->behavior_dev));

    zmk_behavior_queue_add(0, triggered_binding, true, cfg->tap_ms);
    zmk_behavior_queue_add(0, triggered_binding, false, 0);

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_sensor_rotate_var_driver_api = {
    .sensor_binding_triggered = on_sensor_binding_triggered};

static int behavior_sensor_rotate_var_init(const struct device *dev) { return 0; };

#define SENSOR_ROTATE_VAR_INST(n)                                                                  \
    static struct behavior_sensor_rotate_var_config behavior_sensor_rotate_var_config_##n = {      \
        .cw_behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(n, bindings, 0)),                       \
        .ccw_behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(n, bindings, 1)),                      \
        .tap_ms = DT_INST_PROP(n, tap_ms),                                                         \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(                                                                         \
        n, behavior_sensor_rotate_var_init, NULL, NULL, &behavior_sensor_rotate_var_config_##n,    \
        APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_sensor_rotate_var_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_ROTATE_VAR_INST)
