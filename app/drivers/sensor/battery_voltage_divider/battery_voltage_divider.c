/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_battery_voltage_divider

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/sensor.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct io_channel_config {
    const char *label;
    uint8_t channel;
};

struct gpio_channel_config {
    const char *label;
    uint8_t pin;
    uint8_t flags;
};

struct bvd_config {
    struct io_channel_config io_channel;
    struct gpio_channel_config power_gpios;
    uint32_t output_ohm;
    uint32_t full_ohm;
};

struct bvd_data {
    const struct device *adc;
    const struct device *gpio;
    struct adc_channel_cfg acc;
    struct adc_sequence as;
    uint16_t adc_raw;
    uint16_t voltage;
    uint8_t state_of_charge;
};

static uint8_t lithium_ion_mv_to_pct(int16_t bat_mv) {
    // Simple linear approximation of a battery based off adafruit's discharge graph:
    // https://learn.adafruit.com/li-ion-and-lipoly-batteries/voltages

    if (bat_mv >= 4200) {
        return 100;
    } else if (bat_mv <= 3450) {
        return 0;
    }

    return bat_mv * 2 / 15 - 459;
}

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

    // Enable power GPIO if present
    if (drv_data->gpio) {
        rc = gpio_pin_set(drv_data->gpio, drv_cfg->power_gpios.pin, 1);

        if (rc != 0) {
            LOG_DBG("Failed to enable ADC power GPIO: %d", rc);
            return rc;
        }

        // wait for any capacitance to charge up
        k_sleep(K_MSEC(10));
    }

    // Read ADC
    rc = adc_read(drv_data->adc, as);
    as->calibrate = false;

    if (rc == 0) {
        int32_t val = drv_data->adc_raw;

        adc_raw_to_millivolts(adc_ref_internal(drv_data->adc), drv_data->acc.gain, as->resolution,
                              &val);

        uint16_t millivolts = val * (uint64_t)drv_cfg->full_ohm / drv_cfg->output_ohm;
        LOG_DBG("ADC raw %d ~ %d mV => %d mV", drv_data->adc_raw, val, millivolts);
        uint8_t percent = lithium_ion_mv_to_pct(millivolts);
        LOG_DBG("Percent: %d", percent);

        drv_data->voltage = millivolts;
        drv_data->state_of_charge = percent;
    } else {
        LOG_DBG("Failed to read ADC: %d", rc);
    }

    // Disable power GPIO if present
    if (drv_data->gpio) {
        int rc2 = gpio_pin_set(drv_data->gpio, drv_cfg->power_gpios.pin, 0);

        if (rc2 != 0) {
            LOG_DBG("Failed to disable ADC power GPIO: %d", rc2);
            return rc2;
        }
    }

    return rc;
}

static int bvd_channel_get(const struct device *dev, enum sensor_channel chan,
                           struct sensor_value *val) {
    struct bvd_data *drv_data = dev->data;

    switch (chan) {
    case SENSOR_CHAN_GAUGE_VOLTAGE:
        val->val1 = drv_data->voltage / 1000;
        val->val2 = (drv_data->voltage % 1000) * 1000U;
        break;

    case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
        val->val1 = drv_data->state_of_charge;
        val->val2 = 0;
        break;

    default:
        return -ENOTSUP;
    }

    return 0;
}

static const struct sensor_driver_api bvd_api = {
    .sample_fetch = bvd_sample_fetch,
    .channel_get = bvd_channel_get,
};

static int bvd_init(const struct device *dev) {
    struct bvd_data *drv_data = dev->data;
    const struct bvd_config *drv_cfg = dev->config;

    drv_data->adc = device_get_binding(drv_cfg->io_channel.label);

    if (drv_data->adc == NULL) {
        LOG_ERR("ADC %s failed to retrieve", drv_cfg->io_channel.label);
        return -ENODEV;
    }

    int rc = 0;

    if (drv_cfg->power_gpios.label) {
        drv_data->gpio = device_get_binding(drv_cfg->power_gpios.label);
        if (drv_data->gpio == NULL) {
            LOG_ERR("Failed to get GPIO %s", drv_cfg->power_gpios.label);
            return -ENODEV;
        }
        rc = gpio_pin_configure(drv_data->gpio, drv_cfg->power_gpios.pin,
                                GPIO_OUTPUT_INACTIVE | drv_cfg->power_gpios.flags);
        if (rc != 0) {
            LOG_ERR("Failed to control feed %s.%u: %d", drv_cfg->power_gpios.label,
                    drv_cfg->power_gpios.pin, rc);
            return rc;
        }
    }

    drv_data->as = (struct adc_sequence){
        .channels = BIT(0),
        .buffer = &drv_data->adc_raw,
        .buffer_size = sizeof(drv_data->adc_raw),
        .oversampling = 4,
        .calibrate = true,
    };

#ifdef CONFIG_ADC_NRFX_SAADC
    drv_data->acc = (struct adc_channel_cfg){
        .gain = ADC_GAIN_1_5,
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

static struct bvd_data bvd_data;
static const struct bvd_config bvd_cfg = {
    .io_channel =
        {
            DT_INST_IO_CHANNELS_LABEL(0),
            DT_INST_IO_CHANNELS_INPUT(0),
        },
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
    .power_gpios =
        {
            DT_INST_GPIO_LABEL(0, power_gpios),
            DT_INST_GPIO_PIN(0, power_gpios),
            DT_INST_GPIO_FLAGS(0, power_gpios),
        },
#endif
    .output_ohm = DT_INST_PROP(0, output_ohms),
    .full_ohm = DT_INST_PROP(0, full_ohms),
};

DEVICE_AND_API_INIT(bvd_dev, DT_INST_LABEL(0), &bvd_init, &bvd_data, &bvd_cfg, POST_KERNEL,
                    CONFIG_SENSOR_INIT_PRIORITY, &bvd_api);
