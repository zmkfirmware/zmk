/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>

struct ec11_config {
    const char *a_label;
    const uint8_t a_pin;
    const uint8_t a_flags;

    const char *b_label;
    const uint8_t b_pin;
    const uint8_t b_flags;

    const uint8_t resolution;
};

struct ec11_data {
    const struct device *a;
    const struct device *b;
    uint8_t ab_state;
    int8_t pulses;
    int8_t ticks;
    int8_t delta;

#ifdef CONFIG_EC11_TRIGGER
    struct gpio_callback a_gpio_cb;
    struct gpio_callback b_gpio_cb;
    const struct device *dev;

    sensor_trigger_handler_t handler;
    const struct sensor_trigger *trigger;

#if defined(CONFIG_EC11_TRIGGER_OWN_THREAD)
    K_THREAD_STACK_MEMBER(thread_stack, CONFIG_EC11_THREAD_STACK_SIZE);
    struct k_sem gpio_sem;
    struct k_thread thread;
#elif defined(CONFIG_EC11_TRIGGER_GLOBAL_THREAD)
    struct k_work work;
#endif

#endif /* CONFIG_EC11_TRIGGER */
};

#ifdef CONFIG_EC11_TRIGGER

int ec11_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                     sensor_trigger_handler_t handler);

int ec11_init_interrupt(const struct device *dev);
#endif
