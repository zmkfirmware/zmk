/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_battery_voltage_divider

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "battery_common.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct io_channel_config {
    uint8_t channel;
};

struct bvd_config {
    struct io_channel_config io_channel;
    struct gpio_dt_spec power;
    uint32_t output_ohm;
    uint32_t full_ohm;
};

struct bvd_data {
    const struct device *adc;
    struct adc_channel_cfg acc;
    struct adc_sequence as;
    struct battery_value value;
};

static int bvd_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct bvd_data *drv_data = dev->data;
    const struct bvd_config *drv_cfg = dev->config;
    struct adc_sequence *as = &drv_data->as;

    // Make sure selected channel is supported
    if (chan != SENSOR_CHAN_GAUGE_VOLTAGE && chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE &&
        chan != SENSOR_CHAN_ALL) {
        LOG_DBG("Selected channel is not supported: %d.", chan);
        return -ENOTSUP;
    }

    int rc = 0;

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
    // Enable power before sampling
    rc = gpio_pin_set_dt(&drv_cfg->power, 1);

    if (rc != 0) {
        LOG_DBG("Failed to enable ADC power GPIO: %d", rc);
        return rc;
    }

    // wait for any capacitance to charge up
    k_sleep(K_MSEC(10));
#endif // DT_INST_NODE_HAS_PROP(0, power_gpios)

    // Read ADC
    rc = adc_read(drv_data->adc, as);
    as->calibrate = false;

    if (rc == 0) {
        int32_t val = drv_data->value.adc_raw;

        adc_raw_to_millivolts(adc_ref_internal(drv_data->adc), drv_data->acc.gain, as->resolution,
                              &val);

        uint16_t millivolts = val * (uint64_t)drv_cfg->full_ohm / drv_cfg->output_ohm;
        LOG_DBG("ADC raw %d ~ %d mV => %d mV", drv_data->value.adc_raw, val, millivolts);
        uint8_t percent = lithium_ion_mv_to_pct(millivolts);
        LOG_DBG("Percent: %d", percent);

        drv_data->value.millivolts = millivolts;
        drv_data->value.state_of_charge = percent;
    } else {
        LOG_DBG("Failed to read ADC: %d", rc);
    }

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
    // Disable power GPIO if present
    int rc2 = gpio_pin_set_dt(&drv_cfg->power, 0);

    if (rc2 != 0) {
        LOG_DBG("Failed to disable ADC power GPIO: %d", rc2);
        return rc2;
    }
#endif // DT_INST_NODE_HAS_PROP(0, power_gpios)

    return rc;
}

static int bvd_channel_get(const struct device *dev, enum sensor_channel chan,
                           struct sensor_value *val) {
    struct bvd_data *drv_data = dev->data;
    return battery_channel_get(&drv_data->value, chan, val);
}

static const struct sensor_driver_api bvd_api = {
    .sample_fetch = bvd_sample_fetch,
    .channel_get = bvd_channel_get,
};

static int bvd_init(const struct device *dev) {
    struct bvd_data *drv_data = dev->data;
    const struct bvd_config *drv_cfg = dev->config;

    if (drv_data->adc == NULL) {
        LOG_ERR("ADC failed to retrieve ADC driver");
        return -ENODEV;
    }

    int rc = 0;

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
    if (!device_is_ready(drv_cfg->power.port)) {
        LOG_ERR("GPIO port for power control is not ready");
        return -ENODEV;
    }
    rc = gpio_pin_configure_dt(&drv_cfg->power, GPIO_OUTPUT_INACTIVE);
    if (rc != 0) {
        LOG_ERR("Failed to control feed %u: %d", drv_cfg->power.pin, rc);
        return rc;
    }
#endif // DT_INST_NODE_HAS_PROP(0, power_gpios)

    drv_data->as = (struct adc_sequence){
        .channels = BIT(0),
        .buffer = &drv_data->value.adc_raw,
        .buffer_size = sizeof(drv_data->value.adc_raw),
        .oversampling = 0,
        .calibrate = true,
    };

#ifdef CONFIG_ADC_NRFX_SAADC
    drv_data->acc = (struct adc_channel_cfg){
        .gain = ADC_GAIN_1_6,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
        .input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + drv_cfg->io_channel.channel,
    };

    drv_data->as.resolution = 12;
#else
#error Unsupported ADC
#endif

    rc = adc_channel_setup(drv_data->adc, &drv_data->acc);
    LOG_DBG("AIN%u setup returned %d", drv_cfg->io_channel.channel, rc);

    return rc;
}

static struct bvd_data bvd_data = {.adc = DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(DT_DRV_INST(0)))};

static const struct bvd_config bvd_cfg = {
    .io_channel =
        {
            DT_IO_CHANNELS_INPUT(DT_DRV_INST(0)),
        },
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
    .power = GPIO_DT_SPEC_INST_GET(0, power_gpios),
#endif
    .output_ohm = DT_INST_PROP(0, output_ohms),
    .full_ohm = DT_INST_PROP(0, full_ohms),
};

DEVICE_DT_INST_DEFINE(0, &bvd_init, NULL, &bvd_data, &bvd_cfg, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &bvd_api);
