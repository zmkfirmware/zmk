/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

struct ec11_config {
    const struct gpio_dt_spec a;
    const struct gpio_dt_spec b;

    const uint16_t steps;
    const uint8_t resolution;
    int32_t debounce_ms;
    int32_t debounce_scan_period_ms;
};

struct ec11_data {
    int8_t pulses;
    int8_t ticks;
    int8_t delta;

    struct gpio_callback a_gpio_cb;
    struct gpio_callback b_gpio_cb;
    const struct device *dev;

#ifdef CONFIG_EC11_TRIGGER
    sensor_trigger_handler_t handler;
    const struct sensor_trigger *trigger;
#endif /* CONFIG_EC11_TRIGGER */

    bool running;       // timer running?
    uint8_t prev_a;     // previous state (debounced)
    uint8_t prev_b;
    uint8_t samples;    // the window size
    uint32_t hist_a;    // the moving window
    uint32_t hist_b;
    struct k_timer debouncer;
};
