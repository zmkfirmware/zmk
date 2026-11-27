/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_gpio_emul_input

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_gpio_emul_input_config {
    struct gpio_dt_spec gpio;
};

static int behavior_gpio_emul_input_set(struct zmk_behavior_binding *binding, bool active) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_gpio_emul_input_config *config = dev->config;
    bool active_low = (config->gpio.dt_flags & GPIO_ACTIVE_LOW) != 0U;

    return gpio_emul_input_set(config->gpio.port, config->gpio.pin, active != active_low);
}

static int on_behavior_gpio_emul_input_pressed(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    return behavior_gpio_emul_input_set(binding, true);
}

static int on_behavior_gpio_emul_input_released(struct zmk_behavior_binding *binding,
                                                struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    return behavior_gpio_emul_input_set(binding, false);
}

static const struct behavior_driver_api behavior_gpio_emul_input_driver_api = {
    .binding_pressed = on_behavior_gpio_emul_input_pressed,
    .binding_released = on_behavior_gpio_emul_input_released,
};

static int behavior_gpio_emul_input_init(const struct device *dev) {
    const struct behavior_gpio_emul_input_config *config = dev->config;

    if (!gpio_is_ready_dt(&config->gpio)) {
        return -ENODEV;
    }

    return 0;
}

#define GPIO_EMUL_INPUT_INST(n)                                                                    \
    static const struct behavior_gpio_emul_input_config behavior_gpio_emul_input_config_##n = {    \
        .gpio = GPIO_DT_SPEC_INST_GET(n, gpios),                                                   \
    };                                                                                             \
                                                                                                   \
    BEHAVIOR_DT_INST_DEFINE(                                                                       \
        n, behavior_gpio_emul_input_init, NULL, NULL, &behavior_gpio_emul_input_config_##n,        \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_gpio_emul_input_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_EMUL_INPUT_INST)

#endif
