/* SPDX-License-Identifier: MIT */
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define CONFIG_ZMK_LED_MOCK 0
#define IS_ENABLED(x) (x)
#define BIT(x) (1U << (x))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define K_FOREVER 0
#define LOG_MODULE_DECLARE(...)
#define LOG_ERR(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define DEVICE_API(type, name) struct led_driver_api name
#define DT_INST_FOREACH_STATUS_OKAY(fn)

struct device {
    const void *config;
    void *data;
    const void *api;
    const char *name;
    bool ready;
};
struct i2c_dt_spec {
    const struct device *bus;
    uint16_t addr;
};
struct gpio_dt_spec {
    const struct device *port;
    uint8_t pin;
};
struct k_mutex {
    int depth;
};
struct led_rgb {
    uint8_t r, g, b;
};
struct led_driver_api {
    int (*on)(const struct device *, uint32_t);
    int (*off)(const struct device *, uint32_t);
    int (*set_brightness)(const struct device *, uint32_t, uint8_t);
    int (*write_channels)(const struct device *, uint32_t, uint32_t, const uint8_t *);
};
struct led_strip_driver_api {
    int (*update_channels)(const struct device *, uint8_t *, size_t);
    int (*update_rgb)(const struct device *, struct led_rgb *, size_t);
    size_t (*length)(const struct device *);
};
static bool device_is_ready(const struct device *dev) { return dev->ready; }
static bool gpio_is_ready_dt(const struct gpio_dt_spec *gpio) { return gpio->port->ready; }
static int gpio_pin_set_dt(const struct gpio_dt_spec *gpio, int value) {
    (void)gpio;
    (void)value;
    return 0;
}
static void k_mutex_init(struct k_mutex *mutex) { mutex->depth = 0; }
static void k_mutex_lock(struct k_mutex *mutex, int timeout) {
    (void)timeout;
    assert(mutex->depth++ == 0);
}
static void k_mutex_unlock(struct k_mutex *mutex) { assert(--mutex->depth == 0); }
static int led_write_channels(const struct device *dev, uint32_t start, uint32_t count,
                              const uint8_t *buf) {
    const struct led_driver_api *api = dev->api;
    return api->write_channels(dev, start, count, buf);
}
static int led_set_brightness(const struct device *dev, uint32_t led, uint8_t value) {
    const struct led_driver_api *api = dev->api;
    return api->set_brightness(dev, led, value);
}
static int led_on(const struct device *dev, uint32_t led) {
    const struct led_driver_api *api = dev->api;
    return api->on(dev, led);
}
static int led_off(const struct device *dev, uint32_t led) {
    const struct led_driver_api *api = dev->api;
    return api->off(dev, led);
}
static int led_strip_update_channels(const struct device *dev, uint8_t *buf, size_t count) {
    const struct led_strip_driver_api *api = dev->api;
    return api->update_channels ? api->update_channels(dev, buf, count) : -ENOSYS;
}
static int led_strip_update_rgb(const struct device *dev, struct led_rgb *buf, size_t count) {
    const struct led_strip_driver_api *api = dev->api;
    return api->update_rgb(dev, buf, count);
}
static size_t led_strip_length(const struct device *dev) {
    const struct led_strip_driver_api *api = dev->api;
    return api->length(dev);
}
static int ext_power_disable_runtime(const struct device *dev);
static int ext_power_enable_runtime(const struct device *dev);
