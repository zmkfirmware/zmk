/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Mock battery driver for testing purposes.
 * This driver simulates a battery that charges and discharges
 * between 0% and 100% state of charge.
 */

#define DT_DRV_COMPAT zmk_battery_mock

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define STEP (7)

struct batt_mock_data {
    int8_t direction;
    int8_t state_of_charge;
};

static int batt_mock_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    if (chan != SENSOR_CHAN_GAUGE_VOLTAGE && chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE &&
        chan != SENSOR_CHAN_ALL) {
        LOG_DBG("Selected channel is not supported: %d.", chan);
        return -ENOTSUP;
    }

    struct batt_mock_data *drv_data = dev->data;

    int8_t charge = drv_data->state_of_charge + (STEP * drv_data->direction);
    if (charge > 100) {
        charge = 200 - charge;
        drv_data->direction = -1;
    } else if (charge < 0) {
        charge = -charge;
        drv_data->direction = 1;
    }
    drv_data->state_of_charge = charge;

    return 0;
}

static int batt_mock_channel_get(const struct device *dev, enum sensor_channel chan,
                                 struct sensor_value *val_out) {
    struct batt_mock_data const *drv_data = dev->data;

    switch (chan) {
    case SENSOR_CHAN_GAUGE_VOLTAGE:
        uint16_t millivolts = drv_data->state_of_charge * 15 / 2 + 569;

        val_out->val1 = millivolts / 1000;
        val_out->val2 = (millivolts % 1000) * 1000U;
        break;

    case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
        val_out->val1 = drv_data->state_of_charge;
        val_out->val2 = 0;
        break;

    default:
        return -ENOTSUP;
    }

    return 0;
}

static const struct sensor_driver_api batt_mock_api = {
    .sample_fetch = batt_mock_sample_fetch,
    .channel_get = batt_mock_channel_get,
};

static int batt_mock_init(const struct device *dev) {
    struct batt_mock_data *drv_data = dev->data;
    drv_data->direction = 1;
    return 0;
}

static struct batt_mock_data batt_mock_data;

DEVICE_DT_INST_DEFINE(0, &batt_mock_init, NULL, &batt_mock_data, NULL, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &batt_mock_api);
