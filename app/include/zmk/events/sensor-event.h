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
    u8_t sensor_number;
    struct device *sensor;
    s64_t timestamp;
};

ZMK_EVENT_DECLARE(sensor_event);