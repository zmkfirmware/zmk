/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "input_gpio.h"

#include <stdlib.h>

static int compare_ports(const void *a, const void *b) {
    const struct input_gpio *gpio_a = a;
    const struct input_gpio *gpio_b = b;

    return gpio_a->spec.port - gpio_b->spec.port;
}

void input_gpio_list_sort_by_port(struct input_gpio_list *list) {
    qsort(list->gpios, list->len, sizeof(list->gpios[0]), compare_ports);
}

int input_gpio_pin_get(const struct input_gpio *gpio, struct input_gpio_port_state *state) {
    if (gpio->spec.port != state->port) {
        state->port = gpio->spec.port;

        const int err = gpio_port_get(state->port, &state->value);
        if (err) {
            return err;
        }
    }

    return (state->value & BIT(gpio->spec.pin)) != 0;
}
