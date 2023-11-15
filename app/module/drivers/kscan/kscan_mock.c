/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_mock

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <dt-bindings/zmk/kscan_mock.h>

struct kscan_mock_data {
    kscan_callback_t callback;

    uint32_t event_index;
    struct k_work_delayable work;
    const struct device *dev;
};

static int kscan_mock_disable_callback(const struct device *dev) {
    struct kscan_mock_data *data = dev->data;

    k_work_cancel_delayable(&data->work);
    return 0;
}

static int kscan_mock_configure(const struct device *dev, kscan_callback_t callback) {
    struct kscan_mock_data *data = dev->data;

    if (!callback) {
        return -EINVAL;
    }

    data->event_index = 0;
    data->callback = callback;

    return 0;
}

#define MOCK_INST_INIT(n)                                                                          \
    struct kscan_mock_config_##n {                                                                 \
        uint32_t events[DT_INST_PROP_LEN(n, events)];                                              \
        bool exit_after;                                                                           \
    };                                                                                             \
    static void kscan_mock_schedule_next_event_##n(const struct device *dev) {                     \
        struct kscan_mock_data *data = dev->data;                                                  \
        const struct kscan_mock_config_##n *cfg = dev->config;                                     \
        if (data->event_index < DT_INST_PROP_LEN(n, events)) {                                     \
            uint32_t ev = cfg->events[data->event_index];                                          \
            LOG_DBG("delaying next keypress: %d", ZMK_MOCK_MSEC(ev));                              \
            k_work_schedule(&data->work, K_MSEC(ZMK_MOCK_MSEC(ev)));                               \
        } else if (cfg->exit_after) {                                                              \
            LOG_DBG("Exiting");                                                                    \
            exit(0);                                                                               \
        }                                                                                          \
    }                                                                                              \
    static void kscan_mock_work_handler_##n(struct k_work *work) {                                 \
        struct k_work_delayable *d_work = k_work_delayable_from_work(work);                        \
        struct kscan_mock_data *data = CONTAINER_OF(d_work, struct kscan_mock_data, work);         \
        const struct kscan_mock_config_##n *cfg = data->dev->config;                               \
        uint32_t ev = cfg->events[data->event_index];                                              \
        LOG_DBG("ev %u row %d column %d state %d\n", ev, ZMK_MOCK_ROW(ev), ZMK_MOCK_COL(ev),       \
                ZMK_MOCK_IS_PRESS(ev));                                                            \
        data->callback(data->dev, ZMK_MOCK_ROW(ev), ZMK_MOCK_COL(ev), ZMK_MOCK_IS_PRESS(ev));      \
        kscan_mock_schedule_next_event_##n(data->dev);                                             \
        data->event_index++;                                                                       \
    }                                                                                              \
    static int kscan_mock_init_##n(const struct device *dev) {                                     \
        struct kscan_mock_data *data = dev->data;                                                  \
        data->dev = dev;                                                                           \
        k_work_init_delayable(&data->work, kscan_mock_work_handler_##n);                           \
        return 0;                                                                                  \
    }                                                                                              \
    static int kscan_mock_enable_callback_##n(const struct device *dev) {                          \
        kscan_mock_schedule_next_event_##n(dev);                                                   \
        return 0;                                                                                  \
    }                                                                                              \
    static const struct kscan_driver_api mock_driver_api_##n = {                                   \
        .config = kscan_mock_configure,                                                            \
        .enable_callback = kscan_mock_enable_callback_##n,                                         \
        .disable_callback = kscan_mock_disable_callback,                                           \
    };                                                                                             \
    static struct kscan_mock_data kscan_mock_data_##n;                                             \
    static const struct kscan_mock_config_##n kscan_mock_config_##n = {                            \
        .events = DT_INST_PROP(n, events), .exit_after = DT_INST_PROP(n, exit_after)};             \
    DEVICE_DT_INST_DEFINE(n, kscan_mock_init_##n, NULL, &kscan_mock_data_##n,                      \
                          &kscan_mock_config_##n, POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,         \
                          &mock_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(MOCK_INST_INIT)
