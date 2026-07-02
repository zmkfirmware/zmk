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
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "ec11.h"

#define FULL_ROTATION 360

LOG_MODULE_REGISTER(EC11, CONFIG_SENSOR_LOG_LEVEL);

static int ec11_read_pin(const struct device *dev, enum ec11_pin pin) {
    const struct ec11_config *drv_cfg = dev->config;

    if (pin == EC11_PIN_A) {
        return gpio_pin_get_dt(&drv_cfg->a);
    } else {
        return gpio_pin_get_dt(&drv_cfg->b);
    }
}

static int ec11_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct ec11_data *drv_data = dev->data;
    const struct ec11_config *drv_cfg = dev->config;
    uint8_t inactive_pin_state;
    uint8_t active_pin_state;
    uint8_t state;
    int8_t delta;
    enum ec11_pin inactive_pin = drv_data->active_pin == EC11_PIN_A ? EC11_PIN_B : EC11_PIN_A;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_ROTATION);

    inactive_pin_state = ec11_read_pin(dev, inactive_pin);
    active_pin_state = drv_data->active_pin_state ^ 1;
    state = active_pin_state << 2 | drv_data->inactive_pin_state << 1 | inactive_pin_state;

    LOG_DBG("State: %01d%01d%01d", (state >> 2 & 1), (state >> 1 & 1), (state >> 0 & 1));

    switch (state) {
    case 0b100:
    case 0b011:
        delta = -1;
        break;
    case 0b000:
    case 0b111:
        delta = 1;
        break;
    case 0b010:
    case 0b101:
        delta = 2;
        break;
    case 0b110:
    case 0b001:
        delta = -2;
        break;
    default:
        LOG_WRN("Invalid state");
        delta = 0;
        break;
    }

    if (drv_data->active_pin == EC11_PIN_B) {
        delta *= -1;
    }

    LOG_DBG("Delta: %d", delta);

    drv_data->pulses += delta;
    drv_data->active_pin = inactive_pin;
    drv_data->inactive_pin_state = active_pin_state;
    drv_data->active_pin_state = inactive_pin_state;

    // TODO: Temporary code for backwards compatibility to support
    // the sensor channel rotation reporting *ticks* instead of delta of degrees.
    // REMOVE ME
    if (drv_cfg->steps == 0) {
        drv_data->ticks = drv_data->pulses / drv_cfg->resolution;
        drv_data->delta = delta;
        drv_data->pulses %= drv_cfg->resolution;
    }

    return 0;
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

static const struct sensor_driver_api ec11_driver_api = {
#ifdef CONFIG_EC11_TRIGGER
    .trigger_set = ec11_trigger_set,
#endif
    .sample_fetch = ec11_sample_fetch,
    .channel_get = ec11_channel_get,
};

int ec11_init(const struct device *dev) {
    struct ec11_data *drv_data = dev->data;
    const struct ec11_config *drv_cfg = dev->config;
    int pin_state;

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
        LOG_DBG("Failed to configure A pin");
        return -EIO;
    }

    if (gpio_pin_configure_dt(&drv_cfg->b, GPIO_INPUT)) {
        LOG_DBG("Failed to configure B pin");
        return -EIO;
    }

#ifdef CONFIG_EC11_TRIGGER
    if (ec11_init_interrupt(dev) < 0) {
        LOG_DBG("Failed to initialize interrupt!");
        return -EIO;
    }
#endif

    drv_data->active_pin = EC11_PIN_A;

    // In the detent position, pin A is stable and we read its value but cannot do that for pin B
    // since its value may be unstable. Instead, for the state machine, the correct value for pin B
    // is the same as pin A
    pin_state = ec11_read_pin(dev, drv_data->active_pin);
    drv_data->inactive_pin_state = pin_state;
    drv_data->active_pin_state = pin_state;

    return 0;
}

#define EC11_INST(n)                                                                               \
    static struct ec11_data ec11_data_##n;                                                         \
    static const struct ec11_config ec11_cfg_##n = {                                               \
        .a = GPIO_DT_SPEC_INST_GET(n, a_gpios),                                                    \
        .b = GPIO_DT_SPEC_INST_GET(n, b_gpios),                                                    \
        .resolution = DT_INST_PROP_OR(n, resolution, 1),                                           \
        .steps = DT_INST_PROP_OR(n, steps, 0),                                                     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, ec11_init, NULL, &ec11_data_##n, &ec11_cfg_##n, POST_KERNEL,          \
                          CONFIG_SENSOR_INIT_PRIORITY, &ec11_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EC11_INST)
