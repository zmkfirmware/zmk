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
#include <drivers/sensor/battery/battery_charging.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct io_channel_config {
    uint8_t channel;
};

struct bvd_config {
    struct io_channel_config io_channel;
    struct gpio_dt_spec power;
    struct gpio_dt_spec chg;
    uint32_t output_ohm;
    uint32_t full_ohm;
};

struct bvd_data {
    const struct device *adc;
    struct adc_channel_cfg acc;
    struct adc_sequence as;
    struct battery_value value;
#if DT_INST_NODE_HAS_PROP(0, chg_gpios)
    const struct device *dev;
    const struct sensor_trigger *data_ready_trigger;
    struct gpio_callback gpio_cb;
    sensor_trigger_handler_t data_ready_handler;
    struct k_work work;
#endif
};

#if DT_INST_NODE_HAS_PROP(0, chg_gpios)
static void set_int(const struct device *dev, const bool en) {
    const struct bvd_config *drv_cfg = dev->config;
    int ret =
        gpio_pin_interrupt_configure_dt(&drv_cfg->chg, en ? GPIO_INT_EDGE_BOTH : GPIO_INT_DISABLE);
    if (ret < 0) {
        LOG_ERR("can't set interrupt");
    }
}

static int bvd_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                           sensor_trigger_handler_t handler) {
    struct bvd_data *drv_data = dev->data;

    set_int(dev, false);
    if (trig->type != SENSOR_TRIG_DATA_READY) {
        return -ENOTSUP;
    }
    drv_data->data_ready_trigger = trig;
    drv_data->data_ready_handler = handler;
    set_int(dev, true);
    return 0;
}

static void bvd_int_cb(const struct device *dev) {
    struct bvd_data *drv_data = dev->data;
    drv_data->data_ready_handler(dev, drv_data->data_ready_trigger);
    LOG_DBG("Setting int on %d", 0);
    set_int(dev, true);
}
#endif

static int bvd_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct bvd_data *drv_data = dev->data;
    const struct bvd_config *drv_cfg = dev->config;
    struct adc_sequence *as = &drv_data->as;

    // Make sure selected channel is supported
    if (chan != SENSOR_CHAN_GAUGE_VOLTAGE && chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE &&
        (enum sensor_channel_bvd)chan != SENSOR_CHAN_CHARGING && chan != SENSOR_CHAN_ALL) {
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

#if DT_INST_NODE_HAS_PROP(0, chg_gpios)
    int raw = gpio_pin_get_dt(&drv_cfg->chg);
    if (raw == -EIO || raw == -EWOULDBLOCK) {
        LOG_DBG("Failed to read chg status: %d", raw);
        return raw;
    } else {
        bool charging = raw;
        LOG_DBG("Charging state: %d", raw);
        drv_data->value.charging = charging;
    }

#endif
    return rc;
}

static int bvd_channel_get(const struct device *dev, enum sensor_channel chan,
                           struct sensor_value *val) {
    struct bvd_data *drv_data = dev->data;
    return battery_channel_get(&drv_data->value, chan, val);
}

#if DT_INST_NODE_HAS_PROP(0, chg_gpios)
static void bvd_work_cb(struct k_work *work) {
    struct bvd_data *drv_data = CONTAINER_OF(work, struct bvd_data, work);
    bvd_int_cb(drv_data->dev);
}
static void bvd_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    struct bvd_data *drv_data = CONTAINER_OF(cb, struct bvd_data, gpio_cb);
    set_int(drv_data->dev, false);
    k_work_submit(&drv_data->work);
}
#endif

static const struct sensor_driver_api bvd_api = {
#if DT_INST_NODE_HAS_PROP(0, chg_gpios)
    .trigger_set = bvd_trigger_set,
#endif
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

#if DT_INST_NODE_HAS_PROP(0, chg_gpios)
    if (!device_is_ready(drv_cfg->chg.port)) {
        LOG_ERR("GPIO port for chg reading is not ready");
        return -ENODEV;
    }
    rc = gpio_pin_configure_dt(&drv_cfg->chg, GPIO_INPUT);
    if (rc != 0) {
        LOG_ERR("Failed to set chg feed %u: %d", drv_cfg->chg.pin, rc);
        return rc;
    }

    drv_data->dev = dev;
    gpio_init_callback(&drv_data->gpio_cb, bvd_gpio_cb, BIT(drv_cfg->chg.pin));
    int ret = gpio_add_callback(drv_cfg->chg.port, &drv_data->gpio_cb);
    if (ret < 0) {
        LOG_ERR("Failed to set chg callback: %d", ret);
        return -EIO;
    }
    k_work_init(&drv_data->work, bvd_work_cb);
#endif // DT_INST_NODE_HAS_PROP(0, chg_gpios)

    drv_data->as = (struct adc_sequence){
        .channels = BIT(0),
        .buffer = &drv_data->value.adc_raw,
        .buffer_size = sizeof(drv_data->value.adc_raw),
        .oversampling = 4,
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
#if DT_INST_NODE_HAS_PROP(0, chg_gpios)
    .chg = GPIO_DT_SPEC_INST_GET(0, chg_gpios),
#endif
    .output_ohm = DT_INST_PROP(0, output_ohms),
    .full_ohm = DT_INST_PROP(0, full_ohms),
};

DEVICE_DT_INST_DEFINE(0, &bvd_init, NULL, &bvd_data, &bvd_cfg, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &bvd_api);
