/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_mock

#include <device.h>
#include <drivers/kscan.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/kscan-mock.h>

#define MATRIX_NODE_ID DT_DRV_INST(0)
#define MATRIX_ROWS DT_PROP(MATRIX_NODE_ID, rows)
#define MATRIX_COLS DT_PROP(MATRIX_NODE_ID, columns)
#define MATRIX_MOCK_EVENT_COUNT DT_PROP_LEN(MATRIX_NODE_ID, events)

struct kscan_mock_config
{
    u32_t events[MATRIX_MOCK_EVENT_COUNT];
};

struct kscan_mock_data
{
    kscan_callback_t callback;

    u8_t event_index;
    struct k_delayed_work work;
    struct device *dev;
};

static void kscan_mock_schedule_next_event(struct device *dev)
{
    struct kscan_mock_data *data = dev->driver_data;
    const struct kscan_mock_config *cfg = dev->config_info;

    if (data->event_index < MATRIX_MOCK_EVENT_COUNT)
    {
        u32_t ev = cfg->events[data->event_index];
        LOG_DBG("delaying next keypress: %d", ZMK_MOCK_MSEC(ev));
        k_delayed_work_submit(&data->work, K_MSEC(ZMK_MOCK_MSEC(ev)));
    }
}

static int kscan_mock_enable_callback(struct device *dev)
{
    struct kscan_mock_data *data = dev->driver_data;
    kscan_mock_schedule_next_event(dev);
    return 0;
}

static int kscan_mock_disable_callback(struct device *dev)
{
    struct kscan_mock_data *data = dev->driver_data;
    const struct kscan_mock_config *cfg = dev->config_info;

    k_delayed_work_cancel(&data->work);
    return 0;
}

static void kscan_mock_work_handler(struct k_work *work)
{
    struct kscan_mock_data *data =
        CONTAINER_OF(work, struct kscan_mock_data, work);
    struct kscan_mock_config *cfg = data->dev->config_info;

    u32_t ev = cfg->events[data->event_index++];
    LOG_DBG("Triggering ev %d\n", ev);
    data->callback(data->dev, ZMK_MOCK_ROW(ev), ZMK_MOCK_COL(ev), ZMK_MOCK_IS_PRESS(ev));
    kscan_mock_schedule_next_event(data->dev);
}

static int kscan_mock_configure(struct device *dev, kscan_callback_t callback)
{
    struct kscan_mock_data *data = dev->driver_data;

    if (!callback)
    {
        return -EINVAL;
    }

    data->event_index = 0;
    data->callback = callback;

    return 0;
}

static int kscan_mock_init(struct device *dev)
{
    struct kscan_mock_data *data = dev->driver_data;
    const struct kscan_mock_config *cfg = dev->config_info;

    printk("Init first event: %d\n", cfg->events[0]);

    int err;

    data->dev = dev;
    k_delayed_work_init(&data->work, kscan_mock_work_handler);

    return 0;
}

static const struct kscan_driver_api mock_driver_api = {
    .config = kscan_mock_configure,
    .enable_callback = kscan_mock_enable_callback,
    .disable_callback = kscan_mock_disable_callback,
};

static const struct kscan_mock_config kscan_mock_config = {
    .events = DT_PROP(MATRIX_NODE_ID, events)};

static struct kscan_mock_data kscan_mock_data;

DEVICE_AND_API_INIT(kscan_mock, DT_INST_LABEL(0), kscan_mock_init,
                    &kscan_mock_data,
                    &kscan_mock_config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &mock_driver_api);
