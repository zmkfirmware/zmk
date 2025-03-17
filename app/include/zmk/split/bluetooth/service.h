/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/events/sensor_event.h>
#include <zmk/sensors.h>

#define ZMK_SPLIT_RUN_BEHAVIOR_DEV_LEN 9

struct sensor_event {
    uint8_t sensor_index;

    uint8_t channel_data_size;
    struct zmk_sensor_channel_data channel_data[ZMK_SENSOR_EVENT_MAX_CHANNELS];
} __packed;

struct zmk_split_run_behavior_data {
    uint8_t position;
    uint8_t source;
    uint8_t state;
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
} __packed;

struct zmk_split_run_behavior_payload {
    struct zmk_split_run_behavior_data data;
    char behavior_dev[ZMK_SPLIT_RUN_BEHAVIOR_DEV_LEN];
} __packed;

struct zmk_split_input_event_payload {
    uint8_t type;
    uint16_t code;
    uint32_t value;
    uint8_t sync;
} __packed;

int zmk_split_bt_position_pressed(uint8_t position);
int zmk_split_bt_position_released(uint8_t position);
int zmk_split_bt_sensor_triggered(uint8_t sensor_index,
                                  const struct zmk_sensor_channel_data channel_data[],
                                  size_t channel_data_size);

int zmk_split_bt_report_input(uint8_t reg, uint8_t type, uint16_t code, int32_t value, bool sync);
