/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT maxim_max17048

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>

#include "max17048.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_max17048);

static int read_register(const struct device *dev, uint8_t reg, uint16_t *value) {

    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    struct max17048_config *config = (struct max17048_config *)dev->config;

    uint8_t data[2] = {0};
    int ret = i2c_burst_read_dt(&config->i2c_bus, reg, &data[0], sizeof(data));
    if (ret != 0) {
        LOG_DBG("i2c_write_read FAIL %d\n", ret);
        return ret;
    }

    // the register values are returned in big endian (MSB first)
    *value = sys_get_be16(data);
    return 0;
}

static int write_register(const struct device *dev, uint8_t reg, uint16_t value) {

    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    struct max17048_config *config = (struct max17048_config *)dev->config;

    uint8_t data[2] = {0};
    sys_put_be16(value, &data[0]);

    return i2c_burst_write_dt(&config->i2c_bus, reg, &data[0], sizeof(data));
}

static int set_rcomp_value(const struct device *dev, uint8_t rcomp_value) {

    struct max17048_drv_data *const drv_data = (struct max17048_drv_data *const)dev->data;
    k_sem_take(&drv_data->lock, K_FOREVER);

    uint16_t tmp = 0;
    int err = read_register(dev, REG_CONFIG, &tmp);
    if (err != 0) {
        goto done;
    }

    tmp = ((uint16_t)rcomp_value << 8) | (tmp & 0xFF);
    err = write_register(dev, REG_CONFIG, tmp);
    if (err != 0) {
        goto done;
    }

    LOG_DBG("set RCOMP to %d", rcomp_value);

done:
    k_sem_give(&drv_data->lock);
    return err;
}

static int set_sleep_enabled(const struct device *dev, bool sleep) {

    struct max17048_drv_data *const drv_data = (struct max17048_drv_data *const)dev->data;
    k_sem_take(&drv_data->lock, K_FOREVER);

    uint16_t tmp = 0;
    int err = read_register(dev, REG_CONFIG, &tmp);
    if (err != 0) {
        goto done;
    }

    if (sleep) {
        tmp |= 0x80;
    } else {
        tmp &= ~0x0080;
    }

    err = write_register(dev, REG_CONFIG, tmp);
    if (err != 0) {
        goto done;
    }

    LOG_DBG("sleep mode %s", sleep ? "enabled" : "disabled");

done:
    k_sem_give(&drv_data->lock);
    return err;
}

static int max17048_sample_fetch(const struct device *dev, enum sensor_channel chan) {

    struct max17048_drv_data *const drv_data = dev->data;
    k_sem_take(&drv_data->lock, K_FOREVER);

    int err = 0;

    if (chan == SENSOR_CHAN_GAUGE_STATE_OF_CHARGE || chan == SENSOR_CHAN_ALL) {
        err = read_register(dev, REG_STATE_OF_CHARGE, &drv_data->raw_state_of_charge);
        if (err != 0) {
            LOG_WRN("failed to read state-of-charge: %d", err);
            goto done;
        }
        LOG_DBG("read soc: %d", drv_data->raw_state_of_charge);

    } else if (chan == SENSOR_CHAN_GAUGE_VOLTAGE || chan == SENSOR_CHAN_ALL) {
        err = read_register(dev, REG_VCELL, &drv_data->raw_vcell);
        if (err != 0) {
            LOG_WRN("failed to read vcell: %d", err);
            goto done;
        }
        LOG_DBG("read vcell: %d", drv_data->raw_vcell);

    } else {
        LOG_DBG("unsupported channel %d", chan);
        err = -ENOTSUP;
    }

done:
    k_sem_give(&drv_data->lock);
    return err;
}

static int max17048_channel_get(const struct device *dev, enum sensor_channel chan,
                                struct sensor_value *val) {
    int err = 0;

    struct max17048_drv_data *const drv_data = dev->data;
    k_sem_take(&drv_data->lock, K_FOREVER);

    struct max17048_drv_data *const data = dev->data;
    unsigned int tmp = 0;

    switch (chan) {
    case SENSOR_CHAN_GAUGE_VOLTAGE:
        // 1250 / 16 = 78.125
        tmp = data->raw_vcell * 1250 / 16;
        val->val1 = tmp / 1000000;
        val->val2 = tmp % 1000000;
        break;

    case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
        val->val1 = (data->raw_state_of_charge >> 8);
        val->val2 = (data->raw_state_of_charge & 0xFF) * 1000000 / 256;
        break;

    default:
        err = -ENOTSUP;
        break;
    }

    k_sem_give(&drv_data->lock);
    return err;
}

static int max17048_init(const struct device *dev) {
    struct max17048_drv_data *drv_data = dev->data;
    const struct max17048_config *config = dev->config;

    if (!device_is_ready(config->i2c_bus.bus)) {
        LOG_WRN("i2c bus not ready!");
        return -EINVAL;
    }

    uint16_t ic_version = 0;
    int err = read_register(dev, REG_VERSION, &ic_version);
    if (err != 0) {
        LOG_WRN("could not get IC version!");
        return err;
    }

    // the functions below need the semaphore, so initialise it here
    k_sem_init(&drv_data->lock, 1, 1);

    // bring the device out of sleep
    set_sleep_enabled(dev, false);

    // set the default rcomp value -- 0x97, as stated in the datasheet
    set_rcomp_value(dev, 0x97);

    LOG_INF("device initialised at 0x%x (version %d)", config->i2c_bus.addr, ic_version);

    return 0;
}

static const struct sensor_driver_api max17048_api_table = {.sample_fetch = max17048_sample_fetch,
                                                            .channel_get = max17048_channel_get};

#define MAX17048_INIT(inst)                                                                        \
    static struct max17048_config max17048_##inst##_config = {.i2c_bus =                           \
                                                                  I2C_DT_SPEC_INST_GET(inst)};     \
                                                                                                   \
    static struct max17048_drv_data max17048_##inst##_drvdata = {                                  \
        .raw_state_of_charge = 0,                                                                  \
        .raw_charge_rate = 0,                                                                      \
        .raw_vcell = 0,                                                                            \
    };                                                                                             \
                                                                                                   \
    /* This has to init after SPI master */                                                        \
    DEVICE_DT_INST_DEFINE(inst, max17048_init, NULL, &max17048_##inst##_drvdata,                   \
                          &max17048_##inst##_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,     \
                          &max17048_api_table);

DT_INST_FOREACH_STATUS_OKAY(MAX17048_INIT)
