/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_sensor_encoder_mock

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct enc_mock_config {
    uint16_t startup_delay;
    uint16_t event_period;
    bool exit_after;
    const int16_t *events;
    size_t events_len;
};

struct enc_mock_data {
    const struct sensor_trigger *trigger;
    sensor_trigger_handler_t handler;

    size_t event_index;
    struct k_work_delayable work;
    const struct device *dev;
};

static void enc_mock_work_cb(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct enc_mock_data *data = CONTAINER_OF(dwork, struct enc_mock_data, work);

    const struct device *dev = data->dev;

    data->handler(dev, data->trigger);
}

static int enc_mock_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                                sensor_trigger_handler_t handler) {
    struct enc_mock_data *drv_data = dev->data;
    const struct enc_mock_config *drv_cfg = dev->config;

    drv_data->trigger = trig;
    drv_data->handler = handler;

    int ret = k_work_schedule(&drv_data->work, K_MSEC(drv_cfg->startup_delay));
    if (ret < 0) {
        LOG_WRN("Failed to schedule next mock sensor event %d", ret);
        return ret;
    }

    return 0;
}

static int enc_mock_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct enc_mock_data *drv_data = dev->data;
    const struct enc_mock_config *drv_cfg = dev->config;

    drv_data->event_index++;

    if (drv_data->event_index < drv_cfg->events_len - 1) {
        k_work_schedule(&drv_data->work, K_MSEC(drv_cfg->event_period));
    } else if (drv_cfg->exit_after) {
        exit(0);
    }
    return 0;
}

static int enc_mock_channel_get(const struct device *dev, enum sensor_channel chan,
                                struct sensor_value *val) {
    struct enc_mock_data *drv_data = dev->data;
    const struct enc_mock_config *drv_cfg = dev->config;

    val->val1 = drv_cfg->events[drv_data->event_index];

    return 0;
}

static const struct sensor_driver_api enc_mock_driver_api = {
    .trigger_set = enc_mock_trigger_set,
    .sample_fetch = enc_mock_sample_fetch,
    .channel_get = enc_mock_channel_get,
};

int enc_mock_init(const struct device *dev) {
    struct enc_mock_data *drv_data = dev->data;

    drv_data->dev = dev;
    drv_data->event_index = -1;

    k_work_init_delayable(&drv_data->work, enc_mock_work_cb);

    return 0;
}

#define ENC_MOCK_INST(n)                                                                           \
    struct enc_mock_data enc_mock_data_##n = {};                                                   \
    const int16_t mock_data_##n[] = DT_INST_PROP(n, events);                                       \
    const struct enc_mock_config enc_mock_cfg_##n = {                                              \
        .events = mock_data_##n,                                                                   \
        .events_len = DT_INST_PROP_LEN(n, events),                                                 \
        .startup_delay = DT_INST_PROP(n, event_startup_delay),                                     \
        .event_period = DT_INST_PROP(n, event_period),                                             \
        .exit_after = DT_INST_PROP(n, exit_after),                                                 \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, enc_mock_init, NULL, &enc_mock_data_##n, &enc_mock_cfg_##n,           \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &enc_mock_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ENC_MOCK_INST)
