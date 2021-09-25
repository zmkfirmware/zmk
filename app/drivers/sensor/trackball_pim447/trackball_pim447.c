/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT pimoroni_trackball_pim447

#include <drivers/i2c.h>
#include <drivers/sensor.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(trackball_pim447);

#define TRACKBALL_PIM447_REG_LEFT   0x04
#define TRACKBALL_PIM447_REG_RIGHT  0x05
#define TRACKBALL_PIM447_REG_UP     0x06
#define TRACKBALL_PIM447_REG_DOWN   0x07
#define TRACKBALL_PIM447_REG_SWITCH 0x08

#define TRACKBALL_PIM447_REG_MIN    TRACKBALL_PIM447_REG_LEFT
#define TRACKBALL_PIM447_REG_MAX    TRACKBALL_PIM447_REG_SWITCH

struct trackball_pim447_data {
    const struct device *i2c_dev;
    int32_t dx;
    int32_t dy;
    int32_t dz;
};

static int trackball_pim447_read_reg(const struct device *dev,
                                     uint8_t reg, uint8_t *value)
{
    struct trackball_pim447_data *data = dev->data;

    if (reg < TRACKBALL_PIM447_REG_MIN || reg > TRACKBALL_PIM447_REG_MAX) {
        return -ENOTSUP;
    }

    int status = i2c_reg_read_byte(data->i2c_dev, DT_INST_REG_ADDR(0),
                                   reg, value);
    if (status < 0) {
        LOG_ERR("Sensor reg read byte failed");
        return status;
    }

    return 0;
}

static int trackball_pim447_read_axis(const struct device *dev,
                                      uint8_t reg_negative,
                                      uint8_t reg_positive,
                                      int32_t *value)
{
    uint8_t value_negative;
    uint8_t value_positive;
    int status;

    status = trackball_pim447_read_reg(dev, reg_negative, &value_negative);
    if (status < 0) {
        return status;
    }

    status = trackball_pim447_read_reg(dev, reg_positive, &value_positive);
    if (status < 0) {
        return status;
    }

    *value = value_positive - value_negative;

    return 0;
}

static int trackball_pim447_sample_fetch(const struct device *dev,
                                         enum sensor_channel chan)
{
    struct trackball_pim447_data *data = dev->data;
    int status;

    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DX) {
        status = trackball_pim447_read_axis(dev,
                                            TRACKBALL_PIM447_REG_LEFT,
                                            TRACKBALL_PIM447_REG_RIGHT,
                                            &data->dx);
        if (status < 0) {
            return status;
        }
    }

    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DY) {
        status = trackball_pim447_read_axis(dev,
                                            TRACKBALL_PIM447_REG_UP,
                                            TRACKBALL_PIM447_REG_DOWN,
                                            &data->dy);
        if (status < 0) {
            return status;
        }
    }

    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DZ) {
        uint8_t value;
        status = trackball_pim447_read_reg(dev,
                                           TRACKBALL_PIM447_REG_SWITCH,
                                           &value);
        if (status < 0) {
            return status;
        }

        data->dz = value;
    }

    return 0;
}

static int trackball_pim447_channel_get(const struct device *dev,
                                        enum sensor_channel chan,
                                        struct sensor_value *val)
{
    struct trackball_pim447_data *data = dev->data;

    /* Not used.  */
    val->val2 = 0;

    switch (chan) {
    case SENSOR_CHAN_POS_DX:
        val->val1 = data->dx;
        break;

    case SENSOR_CHAN_POS_DY:
        val->val1 = data->dy;
        break;

    case SENSOR_CHAN_POS_DZ:
        val->val1 = data->dz;
        break;

    default:
        return -ENOTSUP;
    }

    return 0;
}

static int trackball_pim447_init(const struct device *dev)
{
    struct trackball_pim447_data *data = dev->data;

    data->i2c_dev = device_get_binding(DT_INST_BUS_LABEL(0));
    if (data->i2c_dev == NULL) {
        LOG_ERR("Failed to get I2C device");
        return -EINVAL;
    }

    return 0;
}

static struct trackball_pim447_data trackball_pim447_data;

static const struct sensor_driver_api trackball_pim447_api = {
    .sample_fetch = trackball_pim447_sample_fetch,
    .channel_get  = trackball_pim447_channel_get,
};

DEVICE_DT_INST_DEFINE(0, &trackball_pim447_init, device_pm_control_nop,
                      &trackball_pim447_data, NULL, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &trackball_pim447_api);
