/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT maxim_max17048

#include <device.h>
#include <kernel.h>
#include <sys/util.h>
#include <logging/log.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>

#include "max17048.h"

LOG_MODULE_REGISTER(MAX17048, CONFIG_SENSOR_LOG_LEVEL);

static int read_register(const struct device *dev, uint8_t reg, uint16_t *value) {

    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    struct max17048_drv_data *const drv_data = dev->data;
    uint16_t dev_addr = ((struct max17048_config *)dev->config)->device_addr;

    uint16_t data = 0;
    int ret = i2c_burst_read(drv_data->i2c, dev_addr, reg, (uint8_t *)&data, sizeof(data));
    if (ret != 0) {
        LOG_DBG("i2c_write_read FAIL %d\n", ret);
        return ret;
    }

    *value = sys_le16_to_cpu(data);
    return 0;
}

static int write_register(const struct device *dev, uint8_t reg, uint16_t value) {

    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    struct max17048_drv_data *const drv_data = dev->data;

    uint16_t data = sys_cpu_to_le16(value);
    uint16_t dev_addr = ((struct max17048_config *)dev->config)->device_addr;

    return i2c_burst_write(drv_data->i2c, dev_addr, reg, (uint8_t *)&data, sizeof(data));
}

static int set_rcomp_value(const struct device *dev, uint8_t rcomp_value) {

    uint16_t tmp = 0;
    int err = read_register(dev, REG_CONFIG, &tmp);
    if (err != 0) {
        return err;
    }

    tmp = ((uint16_t)rcomp_value << 8) | (tmp & 0xFF);
    err = write_register(dev, REG_CONFIG, tmp);
    if (err != 0) {
        return err;
    }

    LOG_DBG("set RCOMP to %d", rcomp_value);
    return 0;
}

static int set_sleep_enabled(const struct device *dev, bool sleep) {
    uint16_t tmp = 0;
    int err = read_register(dev, REG_CONFIG, &tmp);
    if (err != 0) {
        return err;
    }

    if (sleep) {
        tmp |= 0x80;
    } else {
        tmp &= ~0x0080;
    }

    err = write_register(dev, REG_CONFIG, tmp);
    if (err != 0) {
        return err;
    }

    LOG_DBG("sleep mode %s", sleep ? "enabled" : "disabled");
    return 0;
}

static int max17048_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct max17048_drv_data *const data = dev->data;

    if (chan != SENSOR_CHAN_GAUGE_VOLTAGE && chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE) {
        LOG_DBG("unsupported channel %d", chan);
        return -ENOTSUP;
    }

    int err = read_register(dev, REG_STATE_OF_CHARGE, &data->raw_state_of_charge);
    if (err != 0) {
        LOG_WRN("failed to read state-of-charge: %d", err);
        return err;
    }

    err = read_register(dev, REG_VCELL, &data->raw_vcell);
    if (err != 0) {
        LOG_WRN("failed to read vcell: %d", err);
        return err;
    }

    LOG_DBG("read values: soc=%d, vcell=%d", data->raw_state_of_charge, data->raw_vcell);

    return 0;
}

static int max17048_channel_get(const struct device *dev, enum sensor_channel chan,
                                struct sensor_value *val) {

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
        return -ENOTSUP;
    }

    return 0;
}

static int max17048_init(const struct device *dev) {
    struct max17048_drv_data *driver_data = dev->data;
    const struct max17048_config *driver_config = dev->config;

    driver_data->i2c = device_get_binding((char *)driver_config->i2c_device_name);
    if (!driver_data->i2c) {
        LOG_DBG("Unable to get i2c device");
        return -ENODEV;
    }

    if (!device_is_ready(driver_data->i2c)) {
        LOG_WRN("i2c bus not ready!");
        return -EINVAL;
    }

    uint16_t ic_version = 0;
    int err = read_register(dev, REG_VERSION, &ic_version);
    if (err != 0) {
        LOG_WRN("could not get IC version!");
        return err;
    }

    // bring the device out of sleep
    set_sleep_enabled(dev, false);

    // set the default rcomp value -- 0x97, as stated in the datasheet
    set_rcomp_value(dev, 0x97);

    LOG_INF("device initialised at 0x%x (i2c=%s) (version %d)", driver_config->device_addr,
            driver_config->i2c_device_name, ic_version);

    return 0;
}

static const struct sensor_driver_api max17048_api_table = {.sample_fetch = max17048_sample_fetch,
                                                            .channel_get = max17048_channel_get};

#define MAX17048_INIT(inst)                                                                        \
    static struct max17048_config max17048_##inst##_config = {                                     \
        .i2c_device_name = DT_INST_BUS_LABEL(inst),                                                \
        .device_addr = DT_INST_REG_ADDR(inst),                                                     \
    };                                                                                             \
                                                                                                   \
    static struct max17048_drv_data max17048_##inst##_drvdata = {};                                \
                                                                                                   \
    /* This has to init after SPI master */                                                        \
    DEVICE_DT_INST_DEFINE(inst, max17048_init, NULL, &max17048_##inst##_drvdata,                   \
                          &max17048_##inst##_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,     \
                          &max17048_api_table);

DT_INST_FOREACH_STATUS_OKAY(MAX17048_INIT)
