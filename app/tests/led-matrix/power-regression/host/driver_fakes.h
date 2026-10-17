/* SPDX-License-Identifier: MIT */
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define CONFIG_ZMK_IS31FL3737 1
#define CONFIG_IS31FL3733 0
#define IS_ENABLED(x) (x)
#define BIT(x) (1U << (x))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define K_FOREVER 0
#define GPIO_OUTPUT_ACTIVE 1
#define LOG_MODULE_DECLARE(...)
#define LOG_MODULE_REGISTER(...)
#define LOG_ERR(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define DEVICE_API(type, name) struct led_driver_api name
#define DT_INST_FOREACH_STATUS_OKAY(fn)
struct device {
    const void *config;
    void *data;
    bool ready;
};
struct i2c_dt_spec {
    const struct device *bus;
    uint16_t addr;
};
struct gpio_dt_spec {
    const struct device *port;
    uint8_t pin;
    uint8_t dt_flags;
};
struct k_mutex {
    int depth;
};
struct led_driver_api {
    int (*on)(const struct device *, uint32_t);
    int (*off)(const struct device *, uint32_t);
    int (*set_brightness)(const struct device *, uint32_t, uint8_t);
    int (*write_channels)(const struct device *, uint32_t, uint32_t, const uint8_t *);
};
static bool device_is_ready(const struct device *dev) { return dev->ready; }
static bool i2c_is_ready_dt(const struct i2c_dt_spec *bus) { return bus->bus->ready; }
static bool gpio_is_ready_dt(const struct gpio_dt_spec *gpio) { return gpio->port->ready; }
static void k_mutex_init(struct k_mutex *mutex) { mutex->depth = 0; }
static void k_mutex_lock(struct k_mutex *mutex, int timeout) {
    (void)timeout;
    assert(mutex->depth++ == 0);
}
static void k_mutex_unlock(struct k_mutex *mutex) { assert(--mutex->depth == 0); }
static int gpio_pin_set_dt(const struct gpio_dt_spec *, int);
static int gpio_pin_configure_dt(const struct gpio_dt_spec *, int);
static int i2c_reg_write_byte_dt(const struct i2c_dt_spec *, uint8_t, uint8_t);
static int i2c_write_dt(const struct i2c_dt_spec *, const uint8_t *, size_t);
