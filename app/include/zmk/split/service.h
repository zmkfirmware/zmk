/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/events/sensor_event.h>
#include <zmk/sensors.h>

#define ZMK_SPLIT_RUN_BEHAVIOR_DEV_LEN 9
#define ZMK_SPLIT_POS_STATE_LEN 16

struct sensor_event {
    uint8_t sensor_index;

    uint8_t channel_data_size;
    struct zmk_sensor_channel_data channel_data[ZMK_SENSOR_EVENT_MAX_CHANNELS];
} __packed;

struct zmk_split_run_behavior_data {
    uint8_t position;
    uint8_t state;
    uint32_t param1;
    uint32_t param2;
} __packed;

struct zmk_split_run_behavior_payload {
    struct zmk_split_run_behavior_data data;
    char behavior_dev[ZMK_SPLIT_RUN_BEHAVIOR_DEV_LEN];
} __packed;

struct zmk_split_run_behavior_payload_wrapper {
    uint8_t source;
    struct zmk_split_run_behavior_payload payload;
};

int zmk_split_position_pressed(uint8_t position);
int zmk_split_position_released(uint8_t position);
int zmk_split_sensor_triggered(uint8_t sensor_index,
                               const struct zmk_sensor_channel_data channel_data[],
                               size_t channel_data_size);

void send_position_state_impl(uint8_t *state, int len);
#if ZMK_KEYMAP_HAS_SENSORS
void send_sensor_state_impl(struct sensor_event *event, int len);
#endif
