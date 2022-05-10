/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file Header file for the MAX7318 driver.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MAX7318_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MAX7318_H_

#include <kernel.h>

#include <drivers/gpio.h>
#include <drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

// Register definitions
#define REG_INPUT_PORTA 0x00
#define REG_INPUT_PORTB 0x01
#define REG_OUTPUT_PORTA 0x02
#define REG_OUTPUT_PORTB 0x03
#define REG_IPOL_PORTA 0x04
#define REG_IPOL_PORTB 0x05
#define REG_CONFIG_PORTA 0x06
#define REG_CONFIG_PORTB 0x07

// Configuration data
struct max7318_config {
    struct gpio_driver_config common;

    const char *const i2c_dev_name;
    const uint16_t device_addr;
};

// Runtime driver data
struct max7318_drv_data {
    // gpio_driver_data needs to be first
    struct gpio_driver_config data;
    const struct device *i2c;

    struct k_sem lock;

    struct {
        uint16_t ipol;
        uint16_t config;
        uint16_t output;
    } reg_cache;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_MCP23017_H_ */
