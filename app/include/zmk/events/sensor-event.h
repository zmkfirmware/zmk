/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event-manager.h>
#include <device.h>

struct sensor_event {
    struct zmk_event_header header;
    uint8_t sensor_number;
    const struct device *sensor;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(sensor_event);