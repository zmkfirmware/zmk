/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>

struct io_channel_config {
    const char *label;
    uint8_t channel;
};

struct joy_config {
    int io_channel;
    const struct device *adc;
    int resolution;
    int min_on;
    int frequency;
    bool reverse;
};

struct joy_data {
    const struct device *adc;
    int setup;
    
    struct adc_channel_cfg acc;
    struct adc_sequence as;
    uint16_t adc_raw;

    int zero_value;
    int value;
    int delta;
    int last_rotate;
    int last_press;

    const struct device *dev;

    sensor_trigger_handler_t handler;
    const struct sensor_trigger *trigger;

    struct k_timer timer;
    struct k_work work;

};

