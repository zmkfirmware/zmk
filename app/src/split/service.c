/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>

#include <zmk/events/sensor_event.h>
#include <zmk/sensors.h>
#include <zmk/split/service.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static uint8_t position_state[ZMK_SPLIT_POS_STATE_LEN];
#if ZMK_KEYMAP_HAS_SENSORS
static struct sensor_event last_sensor_event;
#endif

K_THREAD_STACK_DEFINE(service_q_stack, CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_STACK_SIZE);

struct k_work_q service_work_q;

K_MSGQ_DEFINE(position_state_msgq, sizeof(char[ZMK_SPLIT_POS_STATE_LEN]),
              CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_POSITION_QUEUE_SIZE, 4);

void send_position_state_callback(struct k_work *work) {
    uint8_t state[ZMK_SPLIT_POS_STATE_LEN];

    while (k_msgq_get(&position_state_msgq, &state, K_NO_WAIT) == 0) {
        send_position_state_impl(state, sizeof(state));
    }
};

K_WORK_DEFINE(service_position_notify_work, send_position_state_callback);

int send_position_state() {
    int err = k_msgq_put(&position_state_msgq, position_state, K_MSEC(100));
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Position state message queue full, popping first message and queueing again");
            uint8_t discarded_state[ZMK_SPLIT_POS_STATE_LEN];
            k_msgq_get(&position_state_msgq, &discarded_state, K_NO_WAIT);
            return send_position_state();
        }
        default:
            LOG_WRN("Failed to queue position state to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&service_work_q, &service_position_notify_work);

    return 0;
}

int zmk_split_position_pressed(uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, true);
    return send_position_state();
}

int zmk_split_position_released(uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, false);
    return send_position_state();
}

#if ZMK_KEYMAP_HAS_SENSORS
K_MSGQ_DEFINE(sensor_state_msgq, sizeof(struct sensor_event),
              CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_POSITION_QUEUE_SIZE, 4);

void send_sensor_state_callback(struct k_work *work) {
    while (k_msgq_get(&sensor_state_msgq, &last_sensor_event, K_NO_WAIT) == 0) {
        send_sensor_state_impl(&last_sensor_event, sizeof(last_sensor_event));
    }
};

K_WORK_DEFINE(service_sensor_notify_work, send_sensor_state_callback);

int send_sensor_state(struct sensor_event ev) {
    int err = k_msgq_put(&sensor_state_msgq, &ev, K_MSEC(100));
    if (err) {
        // retry...
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Sensor state message queue full, popping first message and queueing again");
            struct sensor_event discarded_state;
            k_msgq_get(&sensor_state_msgq, &discarded_state, K_NO_WAIT);
            return send_sensor_state(ev);
        }
        default:
            LOG_WRN("Failed to queue sensor state to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&service_work_q, &service_sensor_notify_work);
    return 0;
}

int zmk_split_sensor_triggered(uint8_t sensor_index,
                               const struct zmk_sensor_channel_data channel_data[],
                               size_t channel_data_size) {
    if (channel_data_size > ZMK_SENSOR_EVENT_MAX_CHANNELS) {
        return -EINVAL;
    }

    struct sensor_event ev =
        (struct sensor_event){.sensor_index = sensor_index, .channel_data_size = channel_data_size};
    memcpy(ev.channel_data, channel_data,
           channel_data_size * sizeof(struct zmk_sensor_channel_data));
    return send_sensor_state(ev);
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */

static int service_init(void) {
    static const struct k_work_queue_config queue_config = {
        .name = "Split Peripheral Notification Queue"};
    k_work_queue_start(&service_work_q, service_q_stack, K_THREAD_STACK_SIZEOF(service_q_stack),
                       CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_PRIORITY, &queue_config);

    return 0;
}

SYS_INIT(service_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
