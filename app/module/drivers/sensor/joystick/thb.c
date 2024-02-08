/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/dt-bindings/adc/adc.h>
#define DT_DRV_COMPAT ck_thb

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(thb, CONFIG_SENSOR_LOG_LEVEL);

#define X_AXIS_TO_ADC_CHAN_ID (0)
#define Y_AXIS_TO_ADC_CHAN_ID (1)

#ifdef CONFIG_ADC_NRFX_SAADC
#define ADC_INPUT_POS_OFFSET SAADC_CH_PSELP_PSELP_AnalogInput0
#else
#define ADC_INPUT_POS_OFFSET 0
#endif

struct thb_config {
    // NOTE: we are assuming both channels using the same ADC, this should hold
    // for pretty much all use cases
    uint8_t channel_x;
    uint8_t channel_y;

    uint32_t min_mv;
    uint32_t max_mv;
};

struct thb_data {
    const struct device *adc;
    struct adc_sequence as;
    int16_t xy_raw[2];
#ifdef CONFIG_JOYSTICK_THB_TRIGGER
    sensor_trigger_handler_t trigger_handler;
    struct sensor_trigger trigger;
    int32_t trigger_fs;
    struct k_timer timer;
    struct k_work work;
#endif
};

#ifdef CONFIG_JOYSTICK_THB_TRIGGER_DEDICATED_QUEUE
K_THREAD_STACK_DEFINE(thb_trigger_stack_area, CONFIG_THB_WORKQUEUE_STACK_SIZE);
static struct k_work_q thb_work_q;
static bool is_thb_work_q_ready = false;
#endif // CONFIG_JOYSTICK_THB_TRIGGER

static int thb_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct thb_data *drv_data = dev->data;
    struct adc_sequence *as = &drv_data->as;

    if (chan != SENSOR_CHAN_POS_DX && chan != SENSOR_CHAN_POS_DY && chan != SENSOR_CHAN_ALL) {
        LOG_ERR("Selected channel is not supported: %d.", chan);
        return -ENOTSUP;
    }

    int rc = 0;

    rc = adc_read(drv_data->adc, as);
    // First read is setup as calibration
    as->calibrate = false;

    return rc;
}

static int thb_channel_get(const struct device *dev, enum sensor_channel chan,
                           struct sensor_value *val) {
    struct thb_data *drv_data = dev->data;
    const struct thb_config *drv_cfg = dev->config;
    struct adc_sequence *as = &drv_data->as;

    int32_t x_mv = drv_data->xy_raw[X_AXIS_TO_ADC_CHAN_ID];
    int32_t y_mv = drv_data->xy_raw[Y_AXIS_TO_ADC_CHAN_ID];

    adc_raw_to_millivolts(adc_ref_internal(drv_data->adc), ADC_GAIN_1_3, as->resolution, &x_mv);
    adc_raw_to_millivolts(adc_ref_internal(drv_data->adc), ADC_GAIN_1_3, as->resolution, &y_mv);

    double out = 0.0;
    switch (chan) {
    // convert from millivolt to normalized output in [-1.0, 1.0]
    case SENSOR_CHAN_POS_DX:
        out = 2.0 * x_mv / (drv_cfg->max_mv - drv_cfg->min_mv) - 1.0;
        sensor_value_from_double(val, out);
        break;
    case SENSOR_CHAN_POS_DY:
        out = 2.0 * y_mv / (drv_cfg->max_mv - drv_cfg->min_mv) - 1.0;
        sensor_value_from_double(val, out);
        break;
    case SENSOR_CHAN_ALL:
        out = 2.0 * x_mv / (drv_cfg->max_mv - drv_cfg->min_mv) - 1.0;
        sensor_value_from_double(val, out);
        out = 2.0 * y_mv / (drv_cfg->max_mv - drv_cfg->min_mv) - 1.0;
        sensor_value_from_double(val + 1, out);
        break;
    default:
        return -ENOTSUP;
    }

    return 0;
}

#ifdef CONFIG_JOYSTICK_THB_TRIGGER
static int thb_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                           sensor_trigger_handler_t handler) {
    struct thb_data *drv_data = dev->data;
    enum sensor_channel chan = trig->chan;
    enum sensor_trigger_type type = trig->type;

    if (chan != SENSOR_CHAN_ALL || type != SENSOR_TRIG_DATA_READY) {
        return -ENOTSUP;
    }

    drv_data->trigger = *trig;
    drv_data->trigger_handler = handler;

    return 0;
}

static int thb_attr_set(const struct device *dev, enum sensor_channel chan,
                        enum sensor_attribute attr, const struct sensor_value *val) {
    struct thb_data *drv_data = dev->data;
    uint32_t usec = 0;

    if (chan != SENSOR_CHAN_ALL || attr != SENSOR_ATTR_SAMPLING_FREQUENCY) {
        return -ENOTSUP;
    }

    if (val->val1 > 100000) {
        LOG_DBG("Sample rate should not exceed 100KHz");
        return -EINVAL;
    }

    drv_data->trigger_fs = val->val1;
    if (drv_data->trigger_fs != 0) {
        usec = 1000000UL / drv_data->trigger_fs;
        k_timer_start(&drv_data->timer, K_USEC(usec), K_USEC(usec));
    } else {
        // explicitly setting duration and period to K_NO_WAIT prevents the
        // timer from going off again
        k_timer_start(&drv_data->timer, K_NO_WAIT, K_NO_WAIT);
    }

    return 0;
}

static int thb_attr_get(const struct device *dev, enum sensor_channel chan,
                        enum sensor_attribute attr, struct sensor_value *val) {
    struct thb_data *drv_data = dev->data;

    if (chan != SENSOR_CHAN_ALL || attr != SENSOR_ATTR_SAMPLING_FREQUENCY) {
        return -ENOTSUP;
    }

    val->val1 = drv_data->trigger_fs;
    val->val2 = 0;

    return 0;
}

static void thb_timer_cb(struct k_timer *item) {
    struct thb_data *drv_data = CONTAINER_OF(item, struct thb_data, timer);
#if defined(CONFIG_JOYSTICK_THB_TRIGGER_DEDICATED_QUEUE)
    k_work_submit_to_queue(&thb_work_q, &drv_data->work);
#elif defined(CONFIG_JOYSTICK_THB_TRIGGER_SYSTEM_QUEUE)
    k_work_submit(&drv_data->work);
#endif
}

static void thb_work_fun(struct k_work *item) {
    struct thb_data *drv_data = CONTAINER_OF(item, struct thb_data, work);
    struct device *dev = CONTAINER_OF(&drv_data, struct device, data);

    thb_sample_fetch(dev, SENSOR_CHAN_ALL);

    if (drv_data->trigger_handler) {
        drv_data->trigger_handler(dev, &drv_data->trigger);
    }
}
#endif // CONFIG_JOYSTICK_THB_TRIGGER

static int thb_init(const struct device *dev) {
    struct thb_data *drv_data = dev->data;
    const struct thb_config *drv_cfg = dev->config;

    if (drv_data->adc == NULL) {
        return -ENODEV;
    }

    struct adc_channel_cfg channel_cfg = {
        .gain = ADC_GAIN_1_3,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .channel_id = X_AXIS_TO_ADC_CHAN_ID,
        .input_positive = ADC_INPUT_POS_OFFSET + drv_cfg->channel_x,
    };

    int rc = adc_channel_setup(drv_data->adc, &channel_cfg);
    if (rc < 0) {
        LOG_DBG("AIN%u setup returned %d", drv_cfg->channel_x, rc);
        return rc;
    }

    channel_cfg.channel_id = Y_AXIS_TO_ADC_CHAN_ID;
    channel_cfg.input_positive = ADC_INPUT_POS_OFFSET + drv_cfg->channel_y;

    rc = adc_channel_setup(drv_data->adc, &channel_cfg);
    if (rc < 0) {
        LOG_DBG("AIN%u setup returned %d", drv_cfg->channel_y, rc);
        return rc;
    }

    drv_data->as = (struct adc_sequence){
        .channels = BIT(X_AXIS_TO_ADC_CHAN_ID) | BIT(Y_AXIS_TO_ADC_CHAN_ID),
        .buffer = drv_data->xy_raw,
        .buffer_size = sizeof(drv_data->xy_raw),
        .oversampling = 0,
        .resolution = 12,
        .calibrate = true,
    };

#ifdef CONFIG_JOYSTICK_THB_TRIGGER
    k_timer_init(&drv_data->timer, thb_timer_cb, NULL);
    k_work_init(&drv_data->work, thb_work_fun);
#ifdef CONFIG_JOYSTICK_THB_TRIGGER_DEDICATED_QUEUE
    if (!is_thb_work_q_ready) {
        k_work_queue_start(&thb_work_q, thb_trigger_stack_area,
                           K_THREAD_STACK_SIZEOF(thb_trigger_stack_area),
                           CONFIG_THB_WORKQUEUE_PRIORITY, NULL);
        is_thb_work_q_ready = true;
    }
#endif
#endif

    return rc;
}

static const struct sensor_driver_api thb_driver_api = {
#ifdef CONFIG_JOYSTICK_THB_TRIGGER
    .trigger_set = thb_trigger_set,
    .attr_set = thb_attr_set,
    .attr_get = thb_attr_get,
#endif
    .sample_fetch = thb_sample_fetch,
    .channel_get = thb_channel_get,
};

#define THB_INST(n)                                                                                \
    static struct thb_data thb_data_##n = {                                                        \
        .adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR_BY_NAME(n, x_axis))};                        \
    static const struct thb_config thb_config_##n = {                                              \
        .channel_x = DT_INST_IO_CHANNELS_INPUT_BY_NAME(n, x_axis),                                 \
        .channel_y = DT_INST_IO_CHANNELS_INPUT_BY_NAME(n, y_axis),                                 \
        .max_mv = DT_INST_PROP(n, max_mv),                                                         \
        .min_mv = COND_CODE_0(DT_INST_NODE_HAS_PROP(n, min_mv), (0), (DT_INST_PROP(n, min_mv))),   \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, thb_init, device_pm_control_nop, &thb_data_##n, &thb_config_##n,      \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &thb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(THB_INST)
