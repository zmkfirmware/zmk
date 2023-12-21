/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_gpio_key_behavior_trigger

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <zmk/event_manager.h>
#include <zmk/behavior.h>
#include <zmk/debounce.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct gkbt_config {
    struct zmk_debounce_config debounce_config;
    int32_t debounce_scan_period_ms;
    struct gpio_dt_spec key;
};

struct gkbt_data {
    struct zmk_behavior_binding binding;
    struct zmk_debounce_state debounce_state;
    struct gpio_callback key_callback;
    const struct device *dev;
    struct k_work_delayable update_work;
    uint32_t read_time;
};

static void gkbt_enable_interrupt(const struct device *dev) {
    const struct gkbt_config *config = dev->config;

    gpio_pin_interrupt_configure_dt(&config->key, GPIO_INT_LEVEL_ACTIVE);
}

static void gkbt_disable_interrupt(const struct device *dev) {
    const struct gkbt_config *config = dev->config;

    gpio_pin_interrupt_configure_dt(&config->key, GPIO_INT_DISABLE);
}

static void gkbt_read(const struct device *dev) {
    const struct gkbt_config *config = dev->config;
    struct gkbt_data *data = dev->data;

    zmk_debounce_update(&data->debounce_state, gpio_pin_get_dt(&config->key),
                        config->debounce_scan_period_ms, &config->debounce_config);

    if (zmk_debounce_get_changed(&data->debounce_state)) {
        const bool pressed = zmk_debounce_is_pressed(&data->debounce_state);

        struct zmk_behavior_binding_event event = {.position = INT32_MAX,
                                                   .timestamp = k_uptime_get()};

        if (pressed) {
            behavior_keymap_binding_pressed(&data->binding, event);
        } else {
            behavior_keymap_binding_released(&data->binding, event);
        }
    }

    if (zmk_debounce_is_active(&data->debounce_state)) {
        data->read_time += config->debounce_scan_period_ms;

        k_work_reschedule(&data->update_work, K_TIMEOUT_ABS_MS(data->read_time));
    } else {
        gkbt_enable_interrupt(dev);
    }
}

static void gkbt_update_work(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct gkbt_data *data = CONTAINER_OF(dwork, struct gkbt_data, update_work);
    gkbt_read(data->dev);
}

static void gkbt_gpio_irq_callback(const struct device *port, struct gpio_callback *cb,
                                   const gpio_port_pins_t pin) {
    struct gkbt_data *data = CONTAINER_OF(cb, struct gkbt_data, key_callback);

    gkbt_disable_interrupt(data->dev);

    data->read_time = k_uptime_get();
    k_work_reschedule(&data->update_work, K_NO_WAIT);
}

static void gkbt_wait_for_key_release(const struct device *dev) {
    const struct gkbt_config *config = dev->config;

    while (gpio_pin_get_dt(&config->key)) {
        k_sleep(K_MSEC(100));
    }
}

static int gkbt_init(const struct device *dev) {
    const struct gkbt_config *config = dev->config;
    struct gkbt_data *data = dev->data;

    if (!device_is_ready(config->key.port)) {
        LOG_ERR("GPIO port %s is not ready", config->key.port->name);
        return -ENODEV;
    }

    k_work_init_delayable(&data->update_work, gkbt_update_work);
    data->dev = dev;

    gpio_pin_configure_dt(&config->key, GPIO_INPUT);
    gpio_init_callback(&data->key_callback, gkbt_gpio_irq_callback, BIT(config->key.pin));
    gpio_add_callback(config->key.port, &data->key_callback);

    // Be sure our wakeup key is released before startup continues to avoid wake/sleep loop.
    gkbt_wait_for_key_release(dev);

    gkbt_enable_interrupt(dev);

    return 0;
}

static int gkbt_pm_action(const struct device *dev, enum pm_device_action action) {
    const struct gkbt_config *config = dev->config;
    struct gkbt_data *data = dev->data;

    int ret;

    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        gkbt_disable_interrupt(dev);
        ret = gpio_remove_callback(config->key.port, &data->key_callback);
        break;
    case PM_DEVICE_ACTION_RESUME:
        ret = gpio_add_callback(config->key.port, &data->key_callback);
        gkbt_enable_interrupt(dev);
        break;
    default:
        ret = -ENOTSUP;
        break;
    }

    return ret;
}

#define GKBT_INST(n)                                                                               \
    const struct gkbt_config gkbt_config_##n = {                                                   \
        .key = GPIO_DT_SPEC_GET(DT_INST_PHANDLE(n, key), gpios),                                   \
        .debounce_config =                                                                         \
            {                                                                                      \
                .debounce_press_ms = DT_INST_PROP(n, debounce_press_ms),                           \
                .debounce_release_ms = DT_INST_PROP(n, debounce_release_ms),                       \
            },                                                                                     \
        .debounce_scan_period_ms = DT_INST_PROP(n, debounce_scan_period_ms),                       \
    };                                                                                             \
    struct gkbt_data gkbt_data_##n = {                                                             \
        .binding = ZMK_KEYMAP_EXTRACT_BINDING(0, DT_DRV_INST(n)),                                  \
    };                                                                                             \
    PM_DEVICE_DT_INST_DEFINE(n, gkbt_pm_action);                                                   \
    DEVICE_DT_INST_DEFINE(n, gkbt_init, PM_DEVICE_DT_INST_GET(n), &gkbt_data_##n,                  \
                          &gkbt_config_##n, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,      \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(GKBT_INST)
