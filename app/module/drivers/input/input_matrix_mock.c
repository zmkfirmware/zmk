/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_matrix_mock

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <dt-bindings/zmk/input_matrix_mock.h>

struct input_matrix_mock_config {
    bool exit_after;
    const uint32_t *events;
    size_t events_len;
};

struct input_matrix_mock_data {
    uint32_t event_index;
    struct k_work_delayable work;
    const struct device *dev;
};

static void input_matrix_mock_schedule_next_event(const struct device *dev) {
    struct input_matrix_mock_data *data = dev->data;
    const struct input_matrix_mock_config *cfg = dev->config;
    if (data->event_index < cfg->events_len) {
        uint32_t ev = cfg->events[data->event_index];
        LOG_DBG("delaying next keypress: %d", ZMK_MOCK_MSEC(ev));
        k_work_schedule(&data->work, K_MSEC(ZMK_MOCK_MSEC(ev)));
    } else if (cfg->exit_after) {
        LOG_DBG("Exiting");
        exit(0);
    }
}

static void input_matrix_mock_work_handler(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct input_matrix_mock_data *data = CONTAINER_OF(d_work, struct input_matrix_mock_data, work);
    const struct input_matrix_mock_config *cfg = data->dev->config;
    if (data->event_index >= cfg->events_len) {
        if (cfg->exit_after)
            exit(0);
        else
            return;
    }
    uint32_t ev = cfg->events[data->event_index];
    LOG_DBG("ev %u row %d column %d state %d\n", ev, ZMK_MOCK_ROW(ev), ZMK_MOCK_COL(ev),
            ZMK_MOCK_IS_PRESS(ev));
    input_report_abs(data->dev, INPUT_ABS_X, ZMK_MOCK_COL(ev), false, K_FOREVER);
    input_report_abs(data->dev, INPUT_ABS_Y, ZMK_MOCK_ROW(ev), false, K_FOREVER);
    input_report_key(data->dev, INPUT_BTN_TOUCH, ZMK_MOCK_IS_PRESS(ev), true, K_FOREVER);
    input_matrix_mock_schedule_next_event(data->dev);
    data->event_index++;
}

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int input_matrix_mock_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_RESUME:
	    input_matrix_mock_schedule_next_event(dev);
	    return 0;
    case PM_DEVICE_ACTION_SUSPEND: {
	    struct input_matrix_mock_data *data = dev->data;
	    return k_work_cancel_delayable(&data->work);
    }
    default:
        return -ENOTSUP;
    }
}

#endif


static int input_matrix_mock_init(const struct device *dev) {
    struct input_matrix_mock_data *data = dev->data;
    data->dev = dev;
    k_work_init_delayable(&data->work, input_matrix_mock_work_handler);
#ifdef CONFIG_PM_DEVICE
    pm_device_init_suspended(dev);

#ifdef CONFIG_PM_DEVICE_RUNTIME
    pm_device_runtime_enable(dev);
#endif

#else
    input_matrix_mock_schedule_next_event(dev);
#endif

    return 0;
}

#define MATRIX_MOCK_INST_INIT(n)                                                                          \
    static struct input_matrix_mock_data input_matrix_mock_data_##n;                                             \
    static const uint32_t mock_events_##n[] = DT_INST_PROP(n, events);                             \
    static const struct input_matrix_mock_config input_matrix_mock_config_##n = {                            \
        .events = mock_events_##n, .events_len = DT_INST_PROP_LEN(n, events), .exit_after = DT_INST_PROP(n, exit_after)};             \
    PM_DEVICE_DT_INST_DEFINE(n, input_matrix_mock_pm_action);                                           \
    DEVICE_DT_INST_DEFINE(n, input_matrix_mock_init, PM_DEVICE_DT_INST_GET(n), &input_matrix_mock_data_##n,                      \
                          &input_matrix_mock_config_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,         \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(MATRIX_MOCK_INST_INIT)
