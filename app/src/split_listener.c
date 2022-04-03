/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include <zmk/split/bluetooth/service.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/hid.h>
#include <zmk/sensors.h>
#include <zmk/endpoints.h>

int split_listener(const zmk_event_t *eh) {
    LOG_DBG("");
    const struct zmk_position_state_changed *pos_ev;
    if ((pos_ev = as_zmk_position_state_changed(eh)) != NULL) {
        if (pos_ev != NULL) {
            if (pos_ev->state) {
                return zmk_split_bt_position_pressed(pos_ev->position);
            } else {
                return zmk_split_bt_position_released(pos_ev->position);
            }
        }
    }

#if ZMK_KEYMAP_HAS_SENSORS
    const struct zmk_sensor_event *sensor_ev;
    if ((sensor_ev = as_zmk_sensor_event(eh)) != NULL) {
        if (sensor_ev != NULL) {
            return zmk_split_bt_sensor_triggered(sensor_ev->sensor_number, sensor_ev->value);
        }
    }
#endif /* ZMK_KEYMAP_HAS_SENSORS */
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(split_listener, split_listener);
ZMK_SUBSCRIPTION(split_listener, zmk_position_state_changed);

#if ZMK_KEYMAP_HAS_SENSORS
ZMK_SUBSCRIPTION(split_listener, zmk_sensor_event);
#endif /* ZMK_KEYMAP_HAS_SENSORS */
