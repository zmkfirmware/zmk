/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate_layer_to

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <drivers/sensor.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_sensor_rotate_layer_to_init(const struct device *dev)
{
    return 0;
};

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
                                       const struct device *sensor, int64_t timestamp)
{
    struct sensor_value value;
    int err;

    err = sensor_channel_get(sensor, SENSOR_CHAN_ROTATION, &value);

    if (err)
    {
        LOG_WRN("Failed to get sensor rotation value: %d", err);
        return err;
    }

    switch (value.val1)
    {
    case 1:
        zmk_keymap_layer_to(binding->param1);
        break;
    case -1:
        zmk_keymap_layer_to(binding->param2);
        break;
    default:
        return -ENOTSUP;
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_sensor_rotate_layer_to_driver_api = {
    .sensor_binding_triggered = on_sensor_binding_triggered};

#define KP_INST(n)                                                                                \
    DEVICE_DT_INST_DEFINE(n, behavior_sensor_rotate_layer_to_init, NULL, NULL, NULL, APPLICATION, \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                    \
                          &behavior_sensor_rotate_layer_to_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
