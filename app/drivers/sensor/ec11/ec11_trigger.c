/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT alps_ec11

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "ec11.h"

extern struct ec11_data ec11_driver;

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(EC11, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_int(const struct device *dev, bool enable) {
    const struct ec11_config *cfg = dev->config;

    LOG_DBG("enabled %s", (enable ? "true" : "false"));

    if (gpio_pin_interrupt_configure_dt(&cfg->a, enable ? GPIO_INT_EDGE_BOTH : GPIO_INT_DISABLE)) {
        LOG_WRN("Unable to set A pin GPIO interrupt");
    }

    if (gpio_pin_interrupt_configure_dt(&cfg->b, enable ? GPIO_INT_EDGE_BOTH : GPIO_INT_DISABLE)) {
        LOG_WRN("Unable to set A pin GPIO interrupt");
    }
}

static void ec11_a_gpio_callback(const struct device *dev, struct gpio_callback *cb,
                                 uint32_t pins) {
    struct ec11_data *drv_data = CONTAINER_OF(cb, struct ec11_data, a_gpio_cb);

    LOG_DBG("");

    setup_int(drv_data->dev, false);

#if defined(CONFIG_EC11_TRIGGER_OWN_THREAD)
    k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_EC11_TRIGGER_GLOBAL_THREAD)
    k_work_submit(&drv_data->work);
#endif
}

static void ec11_b_gpio_callback(const struct device *dev, struct gpio_callback *cb,
                                 uint32_t pins) {
    struct ec11_data *drv_data = CONTAINER_OF(cb, struct ec11_data, b_gpio_cb);

    LOG_DBG("");

    setup_int(drv_data->dev, false);

#if defined(CONFIG_EC11_TRIGGER_OWN_THREAD)
    k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_EC11_TRIGGER_GLOBAL_THREAD)
    k_work_submit(&drv_data->work);
#endif
}

static void ec11_thread_cb(const struct device *dev) {
    struct ec11_data *drv_data = dev->data;

    drv_data->handler(dev, drv_data->trigger);

    setup_int(dev, true);
}

#ifdef CONFIG_EC11_TRIGGER_OWN_THREAD
static void ec11_thread(int dev_ptr, int unused) {
    const struct device *dev = INT_TO_POINTER(dev_ptr);
    struct ec11_data *drv_data = dev->data;

    ARG_UNUSED(unused);

    while (1) {
        k_sem_take(&drv_data->gpio_sem, K_FOREVER);
        ec11_thread_cb(dev);
    }
}
#endif

#ifdef CONFIG_EC11_TRIGGER_GLOBAL_THREAD
static void ec11_work_cb(struct k_work *work) {
    struct ec11_data *drv_data = CONTAINER_OF(work, struct ec11_data, work);

    LOG_DBG("");

    ec11_thread_cb(drv_data->dev);
}
#endif

int ec11_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                     sensor_trigger_handler_t handler) {
    struct ec11_data *drv_data = dev->data;

    setup_int(dev, false);

    k_msleep(5);

    drv_data->trigger = trig;
    drv_data->handler = handler;

    setup_int(dev, true);

    return 0;
}

int ec11_init_interrupt(const struct device *dev) {
    struct ec11_data *drv_data = dev->data;
    const struct ec11_config *drv_cfg = dev->config;

    drv_data->dev = dev;
    /* setup gpio interrupt */

    gpio_init_callback(&drv_data->a_gpio_cb, ec11_a_gpio_callback, BIT(drv_cfg->a.pin));

    if (gpio_add_callback(drv_cfg->a.port, &drv_data->a_gpio_cb) < 0) {
        LOG_DBG("Failed to set A callback!");
        return -EIO;
    }

    gpio_init_callback(&drv_data->b_gpio_cb, ec11_b_gpio_callback, BIT(drv_cfg->b.pin));

    if (gpio_add_callback(drv_cfg->b.port, &drv_data->b_gpio_cb) < 0) {
        LOG_DBG("Failed to set B callback!");
        return -EIO;
    }

#if defined(CONFIG_EC11_TRIGGER_OWN_THREAD)
    k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

    k_thread_create(&drv_data->thread, drv_data->thread_stack, CONFIG_EC11_THREAD_STACK_SIZE,
                    (k_thread_entry_t)ec11_thread, dev, 0, NULL,
                    K_PRIO_COOP(CONFIG_EC11_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_EC11_TRIGGER_GLOBAL_THREAD)
    k_work_init(&drv_data->work, ec11_work_cb);
#endif

    return 0;
}
