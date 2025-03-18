/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/drivers/sensor.h>

#define ZMK_KEYMAP_SENSORS_NODE DT_INST(0, zmk_keymap_sensors)
#define ZMK_KEYMAP_HAS_SENSORS DT_NODE_HAS_STATUS(ZMK_KEYMAP_SENSORS_NODE, okay)
#define ZMK_KEYMAP_SENSORS_BY_IDX(idx) DT_PHANDLE_BY_IDX(ZMK_KEYMAP_SENSORS_NODE, sensors, idx)

#if ZMK_KEYMAP_HAS_SENSORS
#define ZMK_KEYMAP_SENSORS_LEN DT_PROP_LEN(ZMK_KEYMAP_SENSORS_NODE, sensors)
#else
#define ZMK_KEYMAP_SENSORS_LEN 0
#endif

const struct zmk_sensor_config *zmk_sensors_get_config_at_index(uint8_t sensor_index);

struct zmk_sensor_config {
    uint16_t triggers_per_rotation;
};

struct zmk_sensor_data {
    struct sensor_value remainder;
    int num_triggers;
};

// This struct is also used for data transfer for splits, so any changes to the size, layout, etc
// is a breaking change for the split GATT service protocol.
struct zmk_sensor_channel_data {
    struct sensor_value value;
    enum sensor_channel channel;
} __packed;

struct zmk_sensor_data *zmk_sensor_get_data(uint32_t sensor_idx);

void zmk_sensor_set_num_triggers(uint32_t sensor_idx, int num_triggers);

void zmk_sensor_set_remainder(uint32_t sensor_idx, struct sensor_value remainder);