/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate_key_press

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <drivers/sensor.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_sensor_rotate_key_press_init(const struct device *dev) { return 0; };

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
                                       const struct device *sensor, int64_t timestamp) {
    struct sensor_value value;
    int err;
    uint32_t keycode;
    LOG_DBG("inc keycode 0x%02X dec keycode 0x%02X", binding->param1, binding->param2);

    err = sensor_channel_get(sensor, SENSOR_CHAN_ROTATION, &value);

    if (err) {
        LOG_WRN("Failed to ge sensor rotation value: %d", err);
        return err;
    }

    switch (value.val1) {
    case 1:
        keycode = binding->param1;
        break;
    case -1:
        keycode = binding->param2;
        break;
    default:
        return -ENOTSUP;
    }

    LOG_DBG("SEND %d", keycode);

    ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(keycode, true, timestamp));

    // TODO: Better way to do this?
    k_msleep(5);

    return ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(keycode, false, timestamp));
}

static const struct behavior_driver_api behavior_sensor_rotate_key_press_driver_api = {
    .sensor_binding_triggered = on_sensor_binding_triggered};

#define KP_INST(n)                                                                                 \
    DEVICE_AND_API_INIT(behavior_sensor_rotate_key_press_##n, DT_INST_LABEL(n),                    \
                        behavior_sensor_rotate_key_press_init, NULL, NULL, APPLICATION,            \
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                       \
                        &behavior_sensor_rotate_key_press_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
