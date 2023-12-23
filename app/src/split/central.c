/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <zephyr/init.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

K_MSGQ_DEFINE(peripheral_event_msgq, sizeof(struct zmk_position_state_changed),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE, 4);

void peripheral_event_work_callback(struct k_work *work) {
    struct zmk_position_state_changed ev;
    while (k_msgq_get(&peripheral_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger key position state change for %d", ev.position);
        raise_zmk_position_state_changed(ev);
    }
}

K_WORK_DEFINE(peripheral_event_work, peripheral_event_work_callback);

void zmk_position_state_change_handle(struct zmk_position_state_changed *ev) {
    k_msgq_put(&peripheral_event_msgq, ev, K_NO_WAIT);
    k_work_submit(&peripheral_event_work);
}

#if ZMK_KEYMAP_HAS_SENSORS
K_MSGQ_DEFINE(peripheral_sensor_event_msgq, sizeof(struct zmk_sensor_event),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE, 4);

void peripheral_sensor_event_work_callback(struct k_work *work) {
    struct zmk_sensor_event ev;
    while (k_msgq_get(&peripheral_sensor_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger sensor change for %d", ev.sensor_index);
        raise_zmk_sensor_event(ev);
    }
}

K_WORK_DEFINE(peripheral_sensor_event_work, peripheral_sensor_event_work_callback);

void zmk_sensor_event_handle(struct zmk_sensor_event *ev) {
    k_msgq_put(&peripheral_sensor_event_msgq, ev, K_NO_WAIT);
    k_work_submit(&peripheral_sensor_event_work);
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */
