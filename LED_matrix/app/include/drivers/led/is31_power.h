/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZMK_APP_INCLUDE_DRIVERS_LED_IS31_POWER_H_
#define ZMK_APP_INCLUDE_DRIVERS_LED_IS31_POWER_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

enum zmk_is31_chip {
    ZMK_IS31_CHIP_NONE = 0,
    ZMK_IS31_CHIP_IS31FL3733,
    ZMK_IS31_CHIP_IS31FL3737,
};

struct zmk_is31_power_config {
    enum zmk_is31_chip chip;
    const struct device *dev;
    struct i2c_dt_spec bus;
    struct gpio_dt_spec sdb;
    uint8_t current_limit;
    uint8_t sync;
};

int zmk_is31_enter_hard_off(const struct zmk_is31_power_config *config);
int zmk_is31_resume_from_hard_off(const struct zmk_is31_power_config *config);
int zmk_is31_blank(const struct zmk_is31_power_config *config, bool blank_en);

#endif /* ZMK_APP_INCLUDE_DRIVERS_LED_IS31_POWER_H_ */
