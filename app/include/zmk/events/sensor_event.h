/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <zmk/event_manager.h>
#include <zmk/sensors.h>

// TODO: Move to Kconfig when we need more than one channel
#define ZMK_SENSOR_EVENT_MAX_CHANNELS 1

struct zmk_sensor_event {
    size_t channel_data_size;
    struct zmk_sensor_channel_data channel_data[ZMK_SENSOR_EVENT_MAX_CHANNELS];

    int64_t timestamp;

    uint8_t sensor_index;
};

ZMK_EVENT_DECLARE(zmk_sensor_event);