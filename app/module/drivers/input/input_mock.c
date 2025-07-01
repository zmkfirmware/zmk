/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_mock

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct input_mock_config {
    uint16_t startup_delay;
    uint16_t event_period;
    bool exit_after;
    const uint32_t *events;
    size_t events_len;
};

struct input_mock_data {
    size_t event_index;
    struct k_work_delayable work;
    const struct device *dev;
};

static void input_mock_work_cb(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct input_mock_data *data = CONTAINER_OF(dwork, struct input_mock_data, work);

    const struct device *dev = data->dev;

    const struct input_mock_config *cfg = dev->config;

    data->event_index++;

    size_t base_idx = data->event_index * 4;

    if (base_idx >= cfg->events_len) {
        if (cfg->exit_after) {
            exit(0);
        } else {
            return;
        }
    }

    input_report(dev, cfg->events[base_idx], cfg->events[base_idx + 1], cfg->events[base_idx + 2],
                 cfg->events[base_idx + 3], K_NO_WAIT);

    k_work_schedule(&data->work, K_MSEC(cfg->event_period));
}

int input_mock_init(const struct device *dev) {
    struct input_mock_data *drv_data = dev->data;
    const struct input_mock_config *drv_cfg = dev->config;

    drv_data->dev = dev;
    drv_data->event_index = -1;

    k_work_init_delayable(&drv_data->work, input_mock_work_cb);

    k_work_schedule(&drv_data->work, K_MSEC(drv_cfg->startup_delay));

    return 0;
}

#define INPUT_MOCK_INST(n)                                                                         \
    static struct input_mock_data input_mock_data_##n = {};                                        \
    static const uint32_t mock_data_##n[] = DT_INST_PROP(n, events);                               \
    static const struct input_mock_config input_mock_cfg_##n = {                                   \
        .events = mock_data_##n,                                                                   \
        .events_len = DT_INST_PROP_LEN(n, events),                                                 \
        .startup_delay = DT_INST_PROP(n, event_startup_delay),                                     \
        .event_period = DT_INST_PROP(n, event_period),                                             \
        .exit_after = DT_INST_PROP(n, exit_after),                                                 \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, input_mock_init, NULL, &input_mock_data_##n, &input_mock_cfg_##n,     \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_MOCK_INST)
