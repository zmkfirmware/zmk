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
#include <zephyr/logging/log.h>

#include "ec11.h"

#define FULL_ROTATION 360

LOG_MODULE_REGISTER(EC11, CONFIG_SENSOR_LOG_LEVEL);

static void ec11_apply_reading(struct ec11_data *drv_data, uint8_t a, uint8_t b) {
    if (a == drv_data->prev_a && b == drv_data->prev_b) {
        LOG_DBG("no state change");
        return;
    }

    bool bwd1 = (drv_data->prev_b != a);
    bool bwd2 = (drv_data->prev_a == b);
    int8_t delta = (bwd1 == bwd2) ? (bwd1 ? -1 : +1) : 0; // delta == 0 implies missing states
    LOG_DBG("state %u%u -> %u%u delta:%d", drv_data->prev_a, drv_data->prev_b, a, b, delta);

    drv_data->pulses += delta;
    drv_data->prev_a = a;
    drv_data->prev_b = b;

    // TODO: Temporary code for backwards compatibility to support
    // the sensor channel rotation reporting *ticks* instead of delta of degrees.
    // REMOVE ME
    const struct ec11_config *drv_cfg = drv_data->dev->config;
    if (drv_cfg->steps == 0) {
        drv_data->ticks = drv_data->pulses / drv_cfg->resolution;
        drv_data->delta = delta;
        drv_data->pulses %= drv_cfg->resolution;
    }

#ifdef CONFIG_EC11_TRIGGER
    // TODO: CONFIG_EC11_TRIGGER_OWN_THREAD, CONFIG_EC11_TRIGGER_GLOBAL_THREAD
    // XXX: zmk_sensors_trigger_handler() already uses a work queue?
    if (delta != 0 && drv_data->handler) {
        drv_data->handler(drv_data->dev, drv_data->trigger);
    }
#endif
}

static int ec11_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    return 0; // nothing to do; driven by interrupts & timer
}

static int ec11_channel_get(const struct device *dev, enum sensor_channel chan,
                            struct sensor_value *val) {
    struct ec11_data *drv_data = dev->data;
    const struct ec11_config *drv_cfg = dev->config;
    int32_t pulses = drv_data->pulses;

    if (chan != SENSOR_CHAN_ROTATION) {
        return -ENOTSUP;
    }

    drv_data->pulses = 0;

    if (drv_cfg->steps > 0) {
        val->val1 = (pulses * FULL_ROTATION) / drv_cfg->steps;
        val->val2 = (pulses * FULL_ROTATION) % drv_cfg->steps;
        if (val->val2 != 0) {
            val->val2 *= 1000000;
            val->val2 /= drv_cfg->steps;
        }
    } else {
        val->val1 = drv_data->ticks;
        val->val2 = drv_data->delta;
    }

    return 0;
}

#ifdef CONFIG_EC11_TRIGGER
static int ec11_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                            sensor_trigger_handler_t handler) {
    struct ec11_data *drv_data = dev->data;
    drv_data->trigger = trig;
    drv_data->handler = handler;
    return 0;
}
#endif

static const struct sensor_driver_api ec11_driver_api = {
#ifdef CONFIG_EC11_TRIGGER
    .trigger_set = ec11_trigger_set,
#endif
    .sample_fetch = ec11_sample_fetch,
    .channel_get = ec11_channel_get,
};

static void ec11_period(struct k_timer *timer_id) {
    struct ec11_data *drv_data = CONTAINER_OF(timer_id, struct ec11_data, debouncer);
    const struct ec11_config *drv_cfg = drv_data->dev->config;

    uint32_t samples_needed = drv_cfg->debounce_ms / drv_cfg->debounce_scan_period_ms;
    samples_needed += !!(drv_cfg->debounce_ms % drv_cfg->debounce_scan_period_ms); // round up

    // add a single reading to the moving window
    drv_data->hist_a = (drv_data->hist_a << 1) | gpio_pin_get_dt(&drv_cfg->a);
    drv_data->hist_b = (drv_data->hist_b << 1) | gpio_pin_get_dt(&drv_cfg->b);
    if (drv_data->samples < samples_needed) {
        drv_data->samples++;
    }

    // histogram from the window
    uint32_t as = drv_data->hist_a;
    uint32_t bs = drv_data->hist_b;
    uint8_t cnts[4] = {0, 0, 0, 0};
    for (uint8_t i = 0; i < drv_data->samples; i++) {
        cnts[((as & 1) << 1) | (bs & 1)]++;
        as >>= 1;
        bs >>= 1;
    }
    LOG_DBG("histogram 00:%u 01:%u 10:%u 11:%u", cnts[0], cnts[1], cnts[2], cnts[3]);

    // check if any state has reached the threshold
    for (uint8_t ab = 0; ab < 4; ab++) {
        if (cnts[ab] >= samples_needed / 2 + 1) { // more than half
            ec11_apply_reading(drv_data, ab >> 1, ab & 1);
            if (cnts[ab] == samples_needed) { // stable for a full window
                LOG_DBG("timer stop");
                drv_data->samples = 0;
                drv_data->running = false;
                k_timer_stop(&drv_data->debouncer);
            }
            break;
        }
    }
}

static void ec11_interrupt_cb_comon(struct ec11_data *drv_data) {
    if (!drv_data->running) {
        LOG_DBG("timer start");
        drv_data->running = true;
        const struct ec11_config *drv_cfg = drv_data->dev->config;
        int32_t ms = drv_cfg->debounce_scan_period_ms;
        k_timer_start(&drv_data->debouncer, K_MSEC(ms), K_MSEC(ms));
    }
}

static void ec11_cb_a(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    ec11_interrupt_cb_comon(CONTAINER_OF(cb, struct ec11_data, a_gpio_cb));
}

static void ec11_cb_b(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    ec11_interrupt_cb_comon(CONTAINER_OF(cb, struct ec11_data, b_gpio_cb));
}

int ec11_init(const struct device *dev) {
    const struct ec11_config *drv_cfg = dev->config;
    LOG_DBG("A: %s %d B: %s %d resolution %d", drv_cfg->a.port->name, drv_cfg->a.pin,
            drv_cfg->b.port->name, drv_cfg->b.pin, drv_cfg->resolution);

    if (!device_is_ready(drv_cfg->a.port)) {
        LOG_ERR("A GPIO device is not ready");
        return -EINVAL;
    }
    if (!device_is_ready(drv_cfg->b.port)) {
        LOG_ERR("B GPIO device is not ready");
        return -EINVAL;
    }

    if (gpio_pin_configure_dt(&drv_cfg->a, GPIO_INPUT)) {
        LOG_ERR("Failed to configure A pin");
        return -EIO;
    }
    if (gpio_pin_configure_dt(&drv_cfg->b, GPIO_INPUT)) {
        LOG_ERR("Failed to configure B pin");
        return -EIO;
    }

    struct ec11_data *drv_data = dev->data; // already zero-initialized
    drv_data->dev = dev;
    drv_data->prev_a = gpio_pin_get_dt(&drv_cfg->a);
    drv_data->prev_b = gpio_pin_get_dt(&drv_cfg->b);

    // enable interrupts
    gpio_init_callback(&drv_data->a_gpio_cb, ec11_cb_a, BIT(drv_cfg->a.pin));
    if (gpio_add_callback(drv_cfg->a.port, &drv_data->a_gpio_cb) < 0) {
        LOG_ERR("Failed to set A callback!");
        return -EIO;
    }
    gpio_init_callback(&drv_data->b_gpio_cb, ec11_cb_b, BIT(drv_cfg->b.pin));
    if (gpio_add_callback(drv_cfg->b.port, &drv_data->b_gpio_cb) < 0) {
        LOG_ERR("Failed to set B callback!");
        return -EIO;
    }
    if (gpio_pin_interrupt_configure_dt(&drv_cfg->a, GPIO_INT_EDGE_BOTH)) {
        LOG_ERR("Unable to set A pin GPIO interrupt");
        return -EIO;
    }
    if (gpio_pin_interrupt_configure_dt(&drv_cfg->b, GPIO_INT_EDGE_BOTH)) {
        LOG_ERR("Unable to set B pin GPIO interrupt");
        return -EIO;
    }

    k_timer_init(&drv_data->debouncer, ec11_period, NULL);
    return 0;
}

#define EC11_INST(n)                                                                               \
    struct ec11_data ec11_data_##n;                                                                \
    const struct ec11_config ec11_cfg_##n = {                                                      \
        .a = GPIO_DT_SPEC_INST_GET(n, a_gpios),                                                    \
        .b = GPIO_DT_SPEC_INST_GET(n, b_gpios),                                                    \
        .resolution = DT_INST_PROP_OR(n, resolution, 1),                                           \
        .steps = DT_INST_PROP_OR(n, steps, 0),                                                     \
        .debounce_ms = DT_INST_PROP_OR(n, debounce_ms, 5),                                         \
        .debounce_scan_period_ms = DT_INST_PROP_OR(n, debounce_scan_period_ms, 1),                 \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, ec11_init, NULL, &ec11_data_##n, &ec11_cfg_##n, POST_KERNEL,          \
                          CONFIG_SENSOR_INIT_PRIORITY, &ec11_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EC11_INST)
