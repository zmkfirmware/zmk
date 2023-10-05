/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PRIVATE_H
#define PRIVATE_H

#include <zephyr/sys/ring_buffer.h>
#include <zmk/events/sensor_event.h>
#include <zmk/sensors.h>

#define POSITION_STATE_DATA_LEN 16
/* event_type, event_payload, CRC16 */
#define MAX_MESSAGE_LEN                                                                            \
    (sizeof(uint8_t) + MAX(POSITION_STATE_DATA_LEN, sizeof(struct sensor_event)) + sizeof(uint16_t))

#define SPLIT_EVENT_POSITION 0
#define SPLIT_EVENT_SENSOR 1

struct sensor_event {
    uint8_t sensor_index;

    uint8_t channel_data_size;
    struct zmk_sensor_channel_data channel_data[ZMK_SENSOR_EVENT_MAX_CHANNELS];
} __packed;

#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
extern struct ring_buf zmk_split_serial_rx_ringbuf;
extern struct k_work zmk_split_serial_rx_work;
#else
void zmk_split_serial_send(const void *const data, const size_t length);
#endif

#endif /* PRIVATE_H */
