
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zmk/behavior_queue.h>
#include <zmk/virtual_key_position.h>

#include "behavior_sensor_rotate_common.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

int zmk_behavior_sensor_rotate_common_accept_data(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
    const struct zmk_sensor_config *sensor_config, size_t channel_data_size,
    const struct zmk_sensor_channel_data *channel_data) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    struct behavior_sensor_rotate_data *data = dev->data;

    const struct sensor_value value = channel_data[0].value;
    int triggers;
    int sensor_index = ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event.position);

    // Some funky special casing for "old encoder behavior" where ticks where reported in val2 only,
    // instead of rotational degrees in val1.
    // REMOVE ME: Remove after a grace period of old ec11 sensor behavior
    if (value.val1 == 0) {
        triggers = value.val2;
    } else {
        struct sensor_value remainder = data->remainder[sensor_index][event.layer];

        remainder.val1 += value.val1;
        remainder.val2 += value.val2;

        if (remainder.val2 >= 1000000 || remainder.val2 <= 1000000) {
            remainder.val1 += remainder.val2 / 1000000;
            remainder.val2 %= 1000000;
        }

        int trigger_degrees = 360 / sensor_config->triggers_per_rotation;
        triggers = remainder.val1 / trigger_degrees;
        remainder.val1 %= trigger_degrees;

        data->remainder[sensor_index][event.layer] = remainder;
    }

    LOG_DBG(
        "val1: %d, val2: %d, remainder: %d/%d triggers: %d inc keycode 0x%02X dec keycode 0x%02X",
        value.val1, value.val2, data->remainder[sensor_index][event.layer].val1,
        data->remainder[sensor_index][event.layer].val2, triggers, binding->param1,
        binding->param2);

    data->triggers[sensor_index][event.layer] = triggers;
    return 0;
}

int zmk_behavior_sensor_rotate_common_process(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event,
                                              enum behavior_sensor_binding_process_mode mode) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_rotate_config *cfg = dev->config;
    struct behavior_sensor_rotate_data *data = dev->data;

    const int sensor_index = ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event.position);

    if (mode != BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_TRIGGER) {
        data->triggers[sensor_index][event.layer] = 0;
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    int triggers = data->triggers[sensor_index][event.layer];

    struct zmk_behavior_binding triggered_binding;
    if (triggers > 0) {
        triggered_binding = cfg->cw_binding;
        if (cfg->override_params) {
            triggered_binding.param1 = binding->param1;
        }
    } else if (triggers < 0) {
        triggers = -triggers;
        triggered_binding = cfg->ccw_binding;
        if (cfg->override_params) {
            triggered_binding.param1 = binding->param2;
        }
    } else {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    LOG_DBG("Sensor binding: %s", binding->behavior_dev);

    for (int i = 0; i < triggers; i++) {
        zmk_behavior_queue_add(event.position, triggered_binding, true, cfg->tap_ms);
        zmk_behavior_queue_add(event.position, triggered_binding, false, 0);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}
