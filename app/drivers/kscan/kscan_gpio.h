/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/sys/util.h>

struct kscan_gpio {
    struct gpio_dt_spec spec;
    /** The index of the GPIO in the devicetree *-gpios array. */
    size_t index;
};

/** GPIO_DT_SPEC_GET_BY_IDX(), but for a struct kscan_gpio. */
#define KSCAN_GPIO_GET_BY_IDX(node_id, prop, idx)                                                  \
    ((struct kscan_gpio){.spec = GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx), .index = idx})

struct kscan_gpio_list {
    struct kscan_gpio *gpios;
    size_t len;
};

/** Define a kscan_gpio_list from a compile-time GPIO array. */
#define KSCAN_GPIO_LIST(gpio_array)                                                                \
    ((struct kscan_gpio_list){.gpios = gpio_array, .len = ARRAY_SIZE(gpio_array)})

struct kscan_gpio_port_state {
    const struct device *port;
    gpio_port_value_t value;
};

/**
 * Sorts a GPIO list by port so it can be used with kscan_gpio_pin_get().
 */
void kscan_gpio_list_sort_by_port(struct kscan_gpio_list *list);

/**
 * Get logical level of an input pin.
 *
 * This is equivalent to gpio_pin_get() except that, when iterating through the
 * pins in a list which is sorted by kscan_gpio_list_sort_by_port(), it only
 * performs one read per port instead of one read per pin.
 *
 * @param gpio The input pin to read.
 * @param state An object to track state between reads. Must be zero-initialized before the first
 * use.
 *
 * @retval 1 If pin logical value is 1 / active.
 * @retval 0 If pin logical value is 0 / inactive.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
int kscan_gpio_pin_get(const struct kscan_gpio *gpio, struct kscan_gpio_port_state *state);
