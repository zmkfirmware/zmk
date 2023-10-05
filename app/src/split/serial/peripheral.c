/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <cobs.h>
#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/sys/crc.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/matrix.h>
#include <zmk/sensors.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "private.h"

K_THREAD_STACK_DEFINE(service_q_stack, CONFIG_ZMK_SPLIT_SERIAL_PERIPHERAL_STACK_SIZE);
static struct k_work_q service_work_q;

static uint8_t position_state[POSITION_STATE_DATA_LEN];
K_MSGQ_DEFINE(position_state_msgq, sizeof(position_state),
              CONFIG_ZMK_SPLIT_SERIAL_PERIPHERAL_POSITION_QUEUE_SIZE, 4);
NET_BUF_SIMPLE_DEFINE_STATIC(message_buf, MAX_MESSAGE_LEN);
static uint8_t tx_buf[COBS_MAX_ENCODED_SIZE(MAX_MESSAGE_LEN) + 1];

static void send_position_handler(struct k_work *work) {
    ARG_UNUSED(work);

    uint8_t state[sizeof(position_state)];
    while (k_msgq_get(&position_state_msgq, &state, K_NO_WAIT) == 0) {
        LOG_INF("send position");

        net_buf_simple_reset(&message_buf);
        net_buf_simple_add_u8(&message_buf, SPLIT_EVENT_POSITION);
        net_buf_simple_add_mem(&message_buf, state, sizeof(state));

        const uint16_t crc = crc16_ccitt(0, message_buf.data, message_buf.len);
        net_buf_simple_add_le16(&message_buf, crc);

        const size_t encoded_length = cobs_encode(message_buf.data, message_buf.len, tx_buf);
        tx_buf[encoded_length] = 0x00;
        zmk_split_serial_send(tx_buf, encoded_length + 1);
    }
};
K_WORK_DEFINE(send_position_work, send_position_handler);

static int queue_sending_position_state(void) {
    int err = k_msgq_put(&position_state_msgq, position_state, K_MSEC(100));
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Position state message queue full, popping first message and queueing again");
            uint8_t discarded_state[sizeof(position_state)];
            k_msgq_get(&position_state_msgq, &discarded_state, K_NO_WAIT);
            return queue_sending_position_state();
        }
        default:
            LOG_WRN("Failed to queue position state to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&service_work_q, &send_position_work);

    return 0;
}

static int position_pressed(const uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, true);
    return queue_sending_position_state();
}

static int position_released(const uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, false);
    return queue_sending_position_state();
}

#if ZMK_KEYMAP_HAS_SENSORS
K_MSGQ_DEFINE(sensor_state_msgq, sizeof(struct sensor_event),
              CONFIG_ZMK_SPLIT_SERIAL_PERIPHERAL_POSITION_QUEUE_SIZE, 4);

static void send_sensor_state_callback(struct k_work *work) {
    while (k_msgq_get(&sensor_state_msgq, &last_sensor_event, K_NO_WAIT) == 0) {
        LOG_INF("send sensor state");
        // TODO
    }
};
K_WORK_DEFINE(service_sensor_notify_work, send_sensor_state_callback);

static int send_sensor_state(struct sensor_event ev) {
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

static int sensor_triggered(uint8_t sensor_index,
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

static int split_listener(const zmk_event_t *eh) {
    LOG_DBG("");
    const struct zmk_position_state_changed *pos_ev;
    if ((pos_ev = as_zmk_position_state_changed(eh)) != NULL) {
        if (pos_ev->state) {
            return position_pressed(pos_ev->position);
        } else {
            return position_released(pos_ev->position);
        }
    }

#if ZMK_KEYMAP_HAS_SENSORS
    const struct zmk_sensor_event *sensor_ev;
    if ((sensor_ev = as_zmk_sensor_event(eh)) != NULL) {
        return sensor_triggered(sensor_ev->sensor_index, sensor_ev->channel_data,
                                sensor_ev->channel_data_size);
    }
#endif /* ZMK_KEYMAP_HAS_SENSORS */

    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(split_listener, split_listener);
ZMK_SUBSCRIPTION(split_listener, zmk_position_state_changed);
#if ZMK_KEYMAP_HAS_SENSORS
ZMK_SUBSCRIPTION(split_listener, zmk_sensor_event);
#endif /* ZMK_KEYMAP_HAS_SENSORS */

static int init(const struct device *_arg) {
    static const struct k_work_queue_config queue_config = {
        .name = "Split Peripheral Notification Queue"};
    k_work_queue_start(&service_work_q, service_q_stack, K_THREAD_STACK_SIZEOF(service_q_stack),
                       CONFIG_ZMK_SPLIT_SERIAL_PERIPHERAL_PRIORITY, &queue_config);

    return 0;
}
SYS_INIT(init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
