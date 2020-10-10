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
#include <zmk/event-manager.h>
#include <zmk/events/keycode-state-changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_sensor_rotate_key_press_config {
    u8_t usage_page;
};
struct behavior_sensor_rotate_key_press_data {};

static int behavior_sensor_rotate_key_press_init(struct device *dev) { return 0; };

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
                                       struct device *sensor) {
    struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_rotate_key_press_config *cfg = dev->config_info;
    struct sensor_value value;
    int err;
    u32_t keycode;
    struct keycode_state_changed *ev;
    LOG_DBG("usage_page 0x%02X inc keycode 0x%02X dec keycode 0x%02X", cfg->usage_page,
            binding->param1, binding->param2);

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

    ev = new_keycode_state_changed();
    ev->usage_page = cfg->usage_page;
    ev->keycode = keycode;
    ev->state = true;
    ZMK_EVENT_RAISE(ev);

    // TODO: Better way to do this?
    k_msleep(5);

    ev = new_keycode_state_changed();
    ev->usage_page = cfg->usage_page;
    ev->keycode = keycode;
    ev->state = false;
    return ZMK_EVENT_RAISE(ev);
}

static const struct behavior_driver_api behavior_sensor_rotate_key_press_driver_api = {
    .sensor_binding_triggered = on_sensor_binding_triggered};

#define KP_INST(n)                                                                                 \
    static const struct behavior_sensor_rotate_key_press_config                                    \
        behavior_sensor_rotate_key_press_config_##n = {.usage_page = DT_INST_PROP(n, usage_page)}; \
    static struct behavior_sensor_rotate_key_press_data behavior_sensor_rotate_key_press_data_##n; \
    DEVICE_AND_API_INIT(                                                                           \
        behavior_sensor_rotate_key_press_##n, DT_INST_LABEL(n),                                    \
        behavior_sensor_rotate_key_press_init, &behavior_sensor_rotate_key_press_data_##n,         \
        &behavior_sensor_rotate_key_press_config_##n, APPLICATION,                                 \
        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_sensor_rotate_key_press_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)