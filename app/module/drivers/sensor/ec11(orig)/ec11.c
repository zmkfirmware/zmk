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

static int ec11_get_ab_state(const struct device *dev) {
    const struct ec11_config *drv_cfg = dev->config;

    return (gpio_pin_get_dt(&drv_cfg->a) << 1) | gpio_pin_get_dt(&drv_cfg->b);
}

static int ec11_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct ec11_data *drv_data = dev->data;
    const struct ec11_config *drv_cfg = dev->config;
    uint8_t val;
    int8_t delta = 0;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_ROTATION);

    val = ec11_get_ab_state(dev);

    LOG_DBG("prev: %d, new: %d", drv_data->ab_state, val);

    const uint8_t *positive_values = (const uint8_t *)drv_cfg->positive_values;
    const uint8_t *negative_values = (const uint8_t *)drv_cfg->negative_values;

    uint8_t pos = (val | (drv_data->ab_state << 2));

    // Check both current and previous positions
    if (drv_cfg->consistent == 1 && drv_data->prev_pos >= 0) {

        for (size_t i = 1; i < drv_cfg->positive_len; i++) {
            if (positive_values[i] == pos && positive_values[i - 1] == drv_data->prev_pos) {
                delta = 1;
                break;
            }
        }

        // Just check position
    } else {
        // Check if pos is in positive_values
        for (size_t i = 0; i < drv_cfg->positive_len; i++) {
            if (positive_values[i] == pos) {
                delta = 1;
                break;
            }
        }

        if (delta != 1) {
            for (size_t i = 0; i < drv_cfg->negative_len; i++) {
                if (negative_values[i] == pos) {
                    delta = -1;
                    break;
                }
            }
        }
    }

    // switch (pos) {
    // case 0b0010: // 2
    // case 0b0100: // 4
    // case 0b1101: // 13
    // case 0b1011: // 11
    //     delta = -1;
    //     break;
    // case 0b0001: // 1
    // case 0b0111: // 7
    // case 0b1110: // 14
    // case 0b1000: // 8
    //     delta = 1;
    //     break;
    // default:
    //     delta = 0;
    //     break;
    // }

    LOG_ERR("VAL: %d, POS: %d, Delta: %d", val, pos, delta);

    drv_data->prev_pos = pos;
    drv_data->pulses += delta;
    drv_data->ab_state = val;

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

    drv_data->ab_state = ec11_get_ab_state(dev);
    drv_data->prev_pos = -1;

    return 0;
}

#define EC11_INST(n)                                                                               \
    struct ec11_data ec11_data_##n;                                                                \
    static const uint8_t ec11_pos_values_##n[] = DT_INST_PROP(n, positive_values);                 \
    static const uint8_t ec11_neg_values_##n[] = DT_INST_PROP(n, negative_values);                 \
    const struct ec11_config ec11_cfg_##n = {                                                      \
        .a = GPIO_DT_SPEC_INST_GET(n, a_gpios),                                                    \
        .b = GPIO_DT_SPEC_INST_GET(n, b_gpios),                                                    \
        .resolution = DT_INST_PROP_OR(n, resolution, 1),                                           \
        .steps = DT_INST_PROP_OR(n, steps, 0),                                                     \
        .positive_values = ec11_pos_values_##n,                                                    \
        .positive_len = DT_INST_PROP_LEN(n, positive_values),                                      \
        .negative_values = ec11_neg_values_##n,                                                    \
        .negative_len = DT_INST_PROP_LEN(n, negative_values),                                      \
        .consistent = DT_INST_PROP_OR(n, consistent, false),                                       \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, ec11_init, NULL, &ec11_data_##n, &ec11_cfg_##n, POST_KERNEL,          \
                          CONFIG_SENSOR_INIT_PRIORITY, &ec11_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EC11_INST)
