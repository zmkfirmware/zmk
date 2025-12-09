
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zmk/sensors.h>
#include <zmk/behavior_queue.h>
#include <zmk/virtual_key_position.h>
#include <zmk/events/position_state_changed.h>
#include "behavior_sensor_rotate_common.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

int zmk_behavior_sensor_rotate_common_process(struct zmk_behavior_binding_event *event) {
    const struct device *dev = zmk_behavior_get_binding(event->behavior_dev);
    const struct behavior_sensor_rotate_config *cfg = dev->config;

    const int sensor_index = ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event->position);

    const struct zmk_sensor_data *data = zmk_sensor_get_data(sensor_index);
    int triggers = data->num_triggers;

    if (triggers > 0) {
        event->behavior_dev = cfg->cw_binding.behavior_dev;
        event->param1 = cfg->override_params ? event->param1 : cfg->cw_binding.param1;
        event->param2 = cfg->cw_binding.param2;
    } else if (triggers < 0) {
        triggers = -triggers;
        event->behavior_dev = cfg->ccw_binding.behavior_dev;
        event->param1 = cfg->override_params ? event->param2 : cfg->ccw_binding.param1;
        event->param2 = cfg->ccw_binding.param2;
    } else {
        return 0;
    }

    LOG_DBG("Sensor binding: %s", event->behavior_dev);

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    // set this value so that it always triggers on central, can be handled more properly later
    event->source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL;
#endif

    for (int i = 0; i < triggers; i++) {
        zmk_behavior_queue_add(event, true, cfg->tap_ms);
        zmk_behavior_queue_add(event, false, 0);
    }
    return 0;
}
