/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_ext_power_mock

#include <stdbool.h>

#include <drivers/ext_power.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct zmk_ext_power_mock_data {
    bool enabled;
    bool persisted_enabled;
};

static int zmk_ext_power_mock_set_state(const struct device *dev, bool enable, bool persist) {
    struct zmk_ext_power_mock_data *data = dev->data;
    bool status_changed = data->enabled != enable;
    bool persisted_changed = data->persisted_enabled != enable;

    if (!status_changed && (!persist || !persisted_changed)) {
        return 0;
    }

    if (persist) {
        data->persisted_enabled = enable;
    }

    data->enabled = enable;
    LOG_INF("EXT power mock %s[%s]=%d", persist ? "persistent" : "runtime", dev->name,
            enable ? 1 : 0);
    return 0;
}

static int zmk_ext_power_mock_enable(const struct device *dev) {
    return zmk_ext_power_mock_set_state(dev, true, true);
}

static int zmk_ext_power_mock_disable(const struct device *dev) {
    return zmk_ext_power_mock_set_state(dev, false, true);
}

static int zmk_ext_power_mock_enable_runtime(const struct device *dev) {
    return zmk_ext_power_mock_set_state(dev, true, false);
}

static int zmk_ext_power_mock_disable_runtime(const struct device *dev) {
    return zmk_ext_power_mock_set_state(dev, false, false);
}

static int zmk_ext_power_mock_get(const struct device *dev) {
    const struct zmk_ext_power_mock_data *data = dev->data;

    return data->enabled ? 1 : 0;
}

static int zmk_ext_power_mock_init(const struct device *dev) {
    struct zmk_ext_power_mock_data *data = dev->data;

    data->enabled = true;
    data->persisted_enabled = true;
    return 0;
}

static const struct ext_power_api zmk_ext_power_mock_api = {
    .enable = zmk_ext_power_mock_enable,
    .disable = zmk_ext_power_mock_disable,
    .enable_runtime = zmk_ext_power_mock_enable_runtime,
    .disable_runtime = zmk_ext_power_mock_disable_runtime,
    .get = zmk_ext_power_mock_get,
};

#define ZMK_EXT_POWER_MOCK_INST(inst)                                                              \
    static struct zmk_ext_power_mock_data zmk_ext_power_mock_data_##inst;                          \
    DEVICE_DT_INST_DEFINE(inst, zmk_ext_power_mock_init, NULL, &zmk_ext_power_mock_data_##inst,   \
                          NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                          &zmk_ext_power_mock_api);

DT_INST_FOREACH_STATUS_OKAY(ZMK_EXT_POWER_MOCK_INST)
