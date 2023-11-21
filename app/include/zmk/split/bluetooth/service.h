/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/events/sensor_event.h>
#include <zmk/sensors.h>

#define ZMK_SPLIT_RUN_BEHAVIOR_DEV_LEN 9
#define ZMK_SPLIT_DATA_XFER_MAX_LEN 16

enum data_tag {
    // RGB state
    DATA_TAG_RGB_STATE,
    // Backlight state
    DATA_TAG_BACKLIGHT_STATE,
    // HID indicators state
    DATA_TAG_HID_INDICATORS_STATE,
    // Keymap state
    DATA_TAG_KEYMAP_STATE,
    // Start of custom tags
    DATA_TAG_CUSTOM_START,
};

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

struct zmk_split_data_xfer_data {
    enum data_tag data_tag;
    uint8_t data_size;
    uint8_t data[ZMK_SPLIT_DATA_XFER_MAX_LEN];
} __packed;

int zmk_split_bt_position_pressed(uint8_t position);
int zmk_split_bt_position_released(uint8_t position);
int zmk_split_bt_sensor_triggered(uint8_t sensor_index,
                                  const struct zmk_sensor_channel_data channel_data[],
                                  size_t channel_data_size);
