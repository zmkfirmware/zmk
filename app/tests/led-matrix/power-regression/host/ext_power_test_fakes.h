/* SPDX-License-Identifier: MIT */
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define CONFIG_SETTINGS 1
#define CONFIG_PM_DEVICE 1
#define CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE 1000
#define IS_ENABLED(x) (x)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define K_MSEC(ms) (ms)
#define LOG_MODULE_DECLARE(...)
#define LOG_DBG(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define LOG_ERR(...) ((void)0)
#define GPIO_OUTPUT_INACTIVE 0
#define DT_HAS_COMPAT_STATUS_OKAY(...) 1
#define DT_DRV_INST(n) (n)
#define DT_INST_PROP_LEN(...) 3
#define DT_INST_PROP_OR(inst, prop, fallback) (fallback)
#define DT_INST_FOREACH_PROP_ELEM_SEP(inst, prop, fn, sep) {.pin = 0}, {.pin = 1}, {.pin = 2}
#define DEVICE_DT_GET(...) (&test_device)
#define SETTINGS_STATIC_HANDLER_DEFINE(...)
#define PM_DEVICE_DT_INST_DEFINE(...)
#define PM_DEVICE_DT_INST_GET(...) NULL
#define DEVICE_DT_INST_DEFINE(...)
struct device {
    const char *name;
    void *data;
    const void *config;
};
static struct device test_device;
struct gpio_dt_spec {
    unsigned pin;
};
struct k_work {
    void (*handler)(struct k_work *);
};
struct k_work_delayable {
    struct k_work work;
    bool pending;
};
typedef int (*settings_read_cb)(void *, void *, size_t);
enum pm_device_action { PM_DEVICE_ACTION_RESUME, PM_DEVICE_ACTION_SUSPEND };
struct ext_power_api {
    int (*enable)(const struct device *);
    int (*disable)(const struct device *);
    int (*enable_runtime)(const struct device *);
    int (*disable_runtime)(const struct device *);
    int (*get)(const struct device *);
};
static int fail_pin;
static bool pin_values[3];
static unsigned pin_calls[3], total_calls, save_schedules, save_writes;
static bool saved_value;
static int gpio_pin_set_dt(const struct gpio_dt_spec *gpio, int value) {
    assert(gpio->pin < 3 && (value == 0 || value == 1));
    pin_calls[gpio->pin]++;
    total_calls++;
    if ((int)gpio->pin == fail_pin)
        return -EIO;
    pin_values[gpio->pin] = value;
    return 0;
}
static int gpio_pin_configure_dt(const struct gpio_dt_spec *gpio, int flags) {
    assert(flags == GPIO_OUTPUT_INACTIVE);
    return gpio_pin_set_dt(gpio, 0);
}
static void k_work_init_delayable(struct k_work_delayable *work, void (*handler)(struct k_work *)) {
    work->work.handler = handler;
}
static int k_work_reschedule(struct k_work_delayable *work, int delay) {
    assert(delay == CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE);
    save_schedules++;
    work->pending = true;
    return 1;
}
static int settings_save_one(const char *path, const void *value, size_t size) {
    assert(strcmp(path, "ext_power/state/test-rail") == 0 && size == sizeof(bool));
    save_writes++;
    memcpy(&saved_value, value, size);
    return 0;
}
static bool settings_name_steq(const char *name, const char *key, const char **next) {
    *next = NULL;
    return strcmp(name, key) == 0;
}
static void k_msleep(uint16_t ms) { (void)ms; }
