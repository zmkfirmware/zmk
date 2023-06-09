
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zmk/behavior_queue.h>

#include "behavior_sensor_rotate_common.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

int zmk_behavior_sensor_rotate_common_trigger(struct zmk_behavior_binding *binding,
                                              const struct device *sensor,
                                              struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_rotate_config *cfg = dev->config;

    struct sensor_value value;

    const int err = sensor_channel_get(sensor, SENSOR_CHAN_ROTATION, &value);

    if (err < 0) {
        LOG_WRN("Failed to get sensor rotation value: %d", err);
        return err;
    }

    struct zmk_behavior_binding triggered_binding;
    switch (value.val1) {
    case 1:
        triggered_binding = cfg->cw_binding;
        if (cfg->override_params) {
            triggered_binding.param1 = binding->param1;
        }
        break;
    case -1:
        triggered_binding = cfg->ccw_binding;
        if (cfg->override_params) {
            triggered_binding.param1 = binding->param2;
        }
        break;
    default:
        return -ENOTSUP;
    }

    LOG_DBG("Sensor binding: %s", binding->behavior_dev);

    zmk_behavior_queue_add(event.position, triggered_binding, true, cfg->tap_ms);
    zmk_behavior_queue_add(event.position, triggered_binding, false, 0);

    return ZMK_BEHAVIOR_OPAQUE;
}
