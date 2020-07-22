/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
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
};

ZMK_EVENT_DECLARE(sensor_event);