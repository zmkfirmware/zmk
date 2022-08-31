/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT joystick

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <stdlib.h>
#include <pm/device.h>
#include "../../../include/drivers/ext_power.h"

#include "joystick.h"

static void zmk_joy_work(struct k_work *work);

LOG_MODULE_REGISTER(JOYSTICK, CONFIG_ZMK_LOG_LEVEL);

static const struct device *ext_power;

static int joy_get_state(const struct device *dev) {
    struct joy_data *drv_data = dev->data;
    const struct joy_config *drv_cfg = dev->config;
    struct adc_sequence *as = &drv_data->as;

    int disable_power = 0;

    if (drv_data->adc==NULL)
        return 0;

    if (ext_power != NULL) {
        int power = ext_power_get(ext_power);
        if (!power) {
            // power is off but must be turned on for ADC
            int rc = ext_power_enable(ext_power);
            if (rc != 0) {
                LOG_ERR("Unable to enable EXT_POWER: %d", rc);
            }
            disable_power = 1;
        }
    }

    int rc = adc_read(drv_data->adc, as);
    as->calibrate = false;
    
    if (disable_power) {
        int rc = ext_power_disable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to disable EXT_POWER: %d", rc);
        }
    }
    
    if (rc == 0) {
        int32_t val = drv_data->adc_raw;
        if (val > 4096) 
            val = 4096;
        return val;
    } else {
	LOG_DBG("Joy failed to read ADC: %d", rc);
        return 0;
    }
}

static int joy_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct joy_data *drv_data = dev->data;
    const struct joy_config *drv_cfg = dev->config;
    int val;

    val = joy_get_state(dev);

    val -= drv_data->zero_value;
    if (drv_cfg->reverse) {
        val = -val;
    }
    drv_data->delta = val - drv_data->value;
    drv_data->value = val;
    
    if (abs (val) >= drv_cfg->min_on) {
        LOG_DBG ("Joystick chan: %d = %d", drv_cfg->io_channel, val);
    }

    return 0;
}

static int joy_channel_get(const struct device *dev, enum sensor_channel chan,
                            struct sensor_value *val) {
    struct joy_data *drv_data = dev->data;
    const struct joy_config *drv_cfg = dev->config;
    
    int value = drv_data->value;
    
    if (chan == SENSOR_CHAN_ROTATION) {
        val->val1 = 0;
        val->val2 = 0;
        if (value >= drv_data->last_rotate + drv_cfg->resolution) {
            drv_data->last_rotate += drv_cfg->resolution;
            val->val1 = 1;
        } else if (value <= drv_data->last_rotate - drv_cfg->resolution) {
            drv_data->last_rotate -= drv_cfg->resolution;
            val->val1 = -1;
        }
        return 0;
    } else if (chan == SENSOR_CHAN_PRESS) {
        val->val1 = 0; // calibration adjusted
        val->val2 = value; // raw value
        if (value >= drv_cfg->min_on) {
            val->val1 =  1 + value - drv_cfg->min_on;
        } else if (value <= -drv_cfg->min_on) {
            val->val1 = -1 + value + drv_cfg->min_on;
        }
        return 0;
    }
    
    return -ENOTSUP;
}

static void zmk_joy_work(struct k_work *work) {
    struct joy_data *drv_data = CONTAINER_OF(work, struct joy_data, work);
    
    if (drv_data->setup) {
        int rc = joy_sample_fetch (drv_data->dev, 0); // I think this might be unnecessary
        if (rc != 0) {
            LOG_DBG("Failed to update joystick value: %d.", rc);
        }
        drv_data->handler (drv_data->dev, drv_data->trigger);
    }
}

static void zmk_joy_timer(struct k_timer *timer) { 
    const struct device *dev = k_timer_user_data_get(timer);
    struct joy_data *drv_data = CONTAINER_OF(timer, struct joy_data, timer);
    k_work_submit(&drv_data->work); 
}

int joy_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                    sensor_trigger_handler_t handler) {
    struct joy_data *drv_data = dev->data;
    const struct joy_config *drv_cfg = dev->config;

    k_timer_stop (&drv_data->timer);
    
    drv_data->trigger = trig;
    drv_data->handler = handler;
    
    k_work_init (&drv_data->work, zmk_joy_work);
    k_timer_init (&drv_data->timer, zmk_joy_timer, NULL);
    k_timer_user_data_set (&drv_data->timer, dev);
    k_timer_start(&drv_data->timer, K_MSEC(1000/drv_cfg->frequency), K_MSEC(1000/drv_cfg->frequency));
    
    return 0;
}


static const struct sensor_driver_api joy_driver_api = {
    .trigger_set = joy_trigger_set,
    .sample_fetch = joy_sample_fetch,
    .channel_get = joy_channel_get,
};


int joy_init(const struct device *dev) {
    struct joy_data *drv_data = dev->data;
    const struct joy_config *drv_cfg = dev->config;

    drv_data->dev = dev;
    drv_data->setup = false;
    drv_data->adc = drv_cfg->adc;
    if (drv_data->adc == NULL) {
        LOG_ERR("Joy: Failed to get pointer to ADC device");
        return -EINVAL;
    }
    
    drv_data->as = (struct adc_sequence){
        .channels = BIT(drv_cfg->io_channel+1), /* Has to be channel +1 because channel 0 is used for the battery */
        .buffer = &drv_data->adc_raw,
        .buffer_size = sizeof(drv_data->adc_raw),
        .oversampling = 4,
        .calibrate = true,
    };

#ifdef CONFIG_ADC_NRFX_SAADC
    drv_data->acc = (struct adc_channel_cfg){
        .gain = ADC_GAIN_1_4,
        .reference = ADC_REF_VDD_1_4,
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + drv_cfg->io_channel,
        .channel_id = drv_cfg->io_channel+1
    };

    drv_data->as.resolution = 12;
#else
#error Unsupported ADC
#endif

    int rc = adc_channel_setup(drv_data->adc, &drv_data->acc);
    LOG_DBG("Joy AIN%u setup returned %d", drv_cfg->io_channel, rc);

    ext_power = device_get_binding("EXT_POWER");
    if (ext_power == NULL) {
        LOG_ERR("Unable to retrieve ext_power device: EXT_POWER");
    }
    
    drv_data->setup = true;

    drv_data->zero_value = drv_data->value = joy_get_state(dev);
    drv_data->delta = 0;
    drv_data->last_rotate = 0;
    
    return 0;
}

#define JOY_INST(n)                                                                              \
    struct joy_data joy_data_##n;                                                                \
    const struct joy_config joy_cfg_##n = {                                                      \
        .io_channel = DT_INST_IO_CHANNELS_INPUT(n),						 \
        .adc = DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(DT_DRV_INST(n))),                               \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, resolution), (1), (DT_INST_PROP(n, resolution))),     \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, min_on), (1), (DT_INST_PROP(n, min_on))),     \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, frequency), (1), (DT_INST_PROP(n, frequency))),     \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, reverse), (1), (DT_INST_PROP(n, reverse))),     \
    };                                                                                           \
    DEVICE_DT_INST_DEFINE(n, joy_init, device_pm_control_nop, &joy_data_##n, &joy_cfg_##n,       \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &joy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(JOY_INST)

