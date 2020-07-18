/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>

struct en11_config {
	const char *a_label;
	const u8_t a_pin;
	const u8_t a_flags;

	const char *b_label;
	const u8_t b_pin;
	const u8_t b_flags;
};

enum en11_pin_state {
	EN11_A_PIN_STATE,
	EN11_B_PIN_STATE
};

struct en11_data {
	struct device *a;
	struct device *b;
	u8_t ab_state;
	s16_t sample;

#ifdef CONFIG_EN11_TRIGGER
	struct device *gpio;
	struct gpio_callback a_gpio_cb;
	struct gpio_callback b_gpio_cb;
	struct device *dev;

	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;

#if defined(CONFIG_EN11_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_EN11_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_EN11_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_EN11_TRIGGER */
};

#ifdef CONFIG_EN11_TRIGGER

int en11_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int en11_init_interrupt(struct device *dev);
#endif
