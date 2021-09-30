/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT alps_ec11

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "ec11.h"

LOG_MODULE_REGISTER(EC11, CONFIG_SENSOR_LOG_LEVEL);

static int ec11_get_ab_state(const struct device *dev) {
    struct ec11_data *drv_data = dev->data;
    const struct ec11_config *drv_cfg = dev->config;

    return (gpio_pin_get(drv_data->a, drv_cfg->a_pin) << 1) |
           gpio_pin_get(drv_data->b, drv_cfg->b_pin);
}

static int ec11_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct ec11_data *drv_data = dev->data;
    const struct ec11_config *drv_cfg = dev->config;
    uint8_t val;
    int8_t delta;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_ROTATION);

    val = ec11_get_ab_state(dev);

    LOG_DBG("prev: %d, new: %d", drv_data->ab_state, val);

    switch (val | (drv_data->ab_state << 2)) {
    case 0b0010:
    case 0b0100:
    case 0b1101:
    case 0b1011:
        delta = -1;
        break;
    case 0b0001:
    case 0b0111:
    case 0b1110:
    case 0b1000:
        delta = 1;
        break;
    default:
        delta = 0;
        break;
    }

    LOG_DBG("Delta: %d", delta);

    drv_data->pulses += delta;
    drv_data->ab_state = val;

    drv_data->ticks = drv_data->pulses / drv_cfg->resolution;
    drv_data->delta = delta;
    drv_data->pulses %= drv_cfg->resolution;

    return 0;
}

static int ec11_channel_get(const struct device *dev, enum sensor_channel chan,
                            struct sensor_value *val) {
    struct ec11_data *drv_data = dev->data;

    if (chan != SENSOR_CHAN_ROTATION) {
        return -ENOTSUP;
    }

    val->val1 = drv_data->ticks;
    val->val2 = drv_data->delta;

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

    LOG_DBG("A: %s %d B: %s %d resolution %d", drv_cfg->a_label, drv_cfg->a_pin, drv_cfg->b_label,
            drv_cfg->b_pin, drv_cfg->resolution);

    drv_data->a = device_get_binding(drv_cfg->a_label);
    if (drv_data->a == NULL) {
        LOG_ERR("Failed to get pointer to A GPIO device");
        return -EINVAL;
    }

    drv_data->b = device_get_binding(drv_cfg->b_label);
    if (drv_data->b == NULL) {
        LOG_ERR("Failed to get pointer to B GPIO device");
        return -EINVAL;
    }

    if (gpio_pin_configure(drv_data->a, drv_cfg->a_pin, drv_cfg->a_flags | GPIO_INPUT)) {
        LOG_DBG("Failed to configure A pin");
        return -EIO;
    }

    if (gpio_pin_configure(drv_data->b, drv_cfg->b_pin, drv_cfg->b_flags | GPIO_INPUT)) {
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

    return 0;
}

#define EC11_INST(n)                                                                               \
    struct ec11_data ec11_data_##n;                                                                \
    const struct ec11_config ec11_cfg_##n = {                                                      \
        .a_label = DT_INST_GPIO_LABEL(n, a_gpios),                                                 \
        .a_pin = DT_INST_GPIO_PIN(n, a_gpios),                                                     \
        .a_flags = DT_INST_GPIO_FLAGS(n, a_gpios),                                                 \
        .b_label = DT_INST_GPIO_LABEL(n, b_gpios),                                                 \
        .b_pin = DT_INST_GPIO_PIN(n, b_gpios),                                                     \
        .b_flags = DT_INST_GPIO_FLAGS(n, b_gpios),                                                 \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, resolution), (1), (DT_INST_PROP(n, resolution))),     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, ec11_init, device_pm_control_nop, &ec11_data_##n, &ec11_cfg_##n,      \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ec11_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EC11_INST)
