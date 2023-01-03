/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <drivers/sensor.h>
#include <zephyr.h>
#include <zmk/event_manager.h>
#include <device.h>
struct zmk_sensor_event {
    uint8_t sensor_number;
    struct sensor_value value;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_sensor_event);