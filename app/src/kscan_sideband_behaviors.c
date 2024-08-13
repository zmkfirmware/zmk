/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_sideband_behaviors

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <zmk/event_manager.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct ksbb_entry {
    struct zmk_behavior_binding binding;
    uint8_t row;
    uint8_t column;
};

struct ksbb_config {
    const struct device *kscan;
    bool auto_enable;
    struct ksbb_entry *entries;
    size_t entries_len;
};

struct ksbb_data {
    kscan_callback_t callback;
    bool enabled;
};

#define GET_KSBB_DEV(n) DEVICE_DT_GET(DT_DRV_INST(n)),

// The kscan callback has no context with it, so we keep a static array of all possible
// KSBBs to check when a kscan callback from the "wrapped" inner kscan fires.
static const struct device *ksbbs[] = {DT_INST_FOREACH_STATUS_OKAY(GET_KSBB_DEV)};

const struct device *find_ksbb_for_inner(const struct device *inner_dev) {
    for (int i = 0; i < ARRAY_SIZE(ksbbs); i++) {
        const struct device *ksbb = ksbbs[i];
        const struct ksbb_config *cfg = ksbb->config;

        if (cfg->kscan == inner_dev) {
            return ksbb;
        }
    }

    return NULL;
}

struct ksbb_entry *find_sideband_behavior(const struct device *dev, uint32_t row, uint32_t column) {
    const struct ksbb_config *cfg = dev->config;

    for (int e = 0; e < cfg->entries_len; e++) {
        struct ksbb_entry *candidate = &cfg->entries[e];

        if (candidate->row == row && candidate->column == column) {
            return candidate;
        }
    }

    return NULL;
}

void ksbb_inner_kscan_callback(const struct device *dev, uint32_t row, uint32_t column,
                               bool pressed) {
    const struct device *ksbb = find_ksbb_for_inner(dev);
    if (ksbb) {
        struct ksbb_data *data = ksbb->data;

        struct ksbb_entry *entry = find_sideband_behavior(ksbb, row, column);
        if (entry) {
            struct zmk_behavior_binding_event event = {.position = INT32_MAX,
                                                       .timestamp = k_uptime_get()};

            if (pressed) {
                behavior_keymap_binding_pressed(&entry->binding, event);
            } else {
                behavior_keymap_binding_released(&entry->binding, event);
            }
        }

        if (data->enabled && data->callback) {
            data->callback(ksbb, row, column, pressed);
        }
    }
}

static int ksbb_configure(const struct device *dev, kscan_callback_t callback) {
    struct ksbb_data *data = dev->data;

    data->callback = callback;

    return 0;
}

static int ksbb_enable(const struct device *dev) {
    struct ksbb_data *data = dev->data;
    const struct ksbb_config *config = dev->config;
    data->enabled = true;

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    if (!pm_device_runtime_is_enabled(dev) && pm_device_runtime_is_enabled(config->kscan)) {
        pm_device_runtime_get(config->kscan);
    }
#elif IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_action_run(config->kscan, PM_DEVICE_ACTION_RESUME);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)

    kscan_config(config->kscan, &ksbb_inner_kscan_callback);
    kscan_enable_callback(config->kscan);

    return 0;
}

static int ksbb_disable(const struct device *dev) {
    struct ksbb_data *data = dev->data;
    const struct ksbb_config *config = dev->config;
    data->enabled = false;

    kscan_disable_callback(config->kscan);

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    if (!pm_device_runtime_is_enabled(dev) && pm_device_runtime_is_enabled(config->kscan)) {
        pm_device_runtime_put(config->kscan);
    }
#elif IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_action_run(config->kscan, PM_DEVICE_ACTION_SUSPEND);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)

    return 0;
}

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int ksbb_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        return ksbb_disable(dev);
    case PM_DEVICE_ACTION_RESUME:
        return ksbb_enable(dev);
    default:
        return -ENOTSUP;
    }
}

#endif // IS_ENABLED(CONFIG_PM_DEVICE)

static int ksbb_init(const struct device *dev) {
    const struct ksbb_config *config = dev->config;

    if (!device_is_ready(config->kscan)) {
        LOG_ERR("kscan %s is not ready", config->kscan->name);
        return -ENODEV;
    }

    if (config->auto_enable) {
#if !IS_ENABLED(CONFIG_PM_DEVICE)
        kscan_config(config->kscan, &ksbb_inner_kscan_callback);
        kscan_enable_callback(config->kscan);
#else
        ksbb_pm_action(dev, PM_DEVICE_ACTION_RESUME);
    } else {
        pm_device_init_suspended(dev);
#endif
    }

    return 0;
}

static const struct kscan_driver_api ksbb_api = {
    .config = ksbb_configure,
    .enable_callback = ksbb_enable,
    .disable_callback = ksbb_disable,
};

#define ENTRY(e)                                                                                   \
    {                                                                                              \
        .row = DT_PROP(e, row), .column = DT_PROP(e, column),                                      \
        .binding = ZMK_KEYMAP_EXTRACT_BINDING(0, e),                                               \
    }

#define KSBB_INST(n)                                                                               \
    static struct ksbb_entry entries_##n[] = {                                                     \
        DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, ENTRY, (, ))};                                    \
    const struct ksbb_config ksbb_config_##n = {                                                   \
        .kscan = DEVICE_DT_GET(DT_INST_PHANDLE(n, kscan)),                                         \
        .auto_enable = DT_INST_PROP_OR(n, auto_enable, false),                                     \
        .entries = entries_##n,                                                                    \
        .entries_len = ARRAY_SIZE(entries_##n),                                                    \
    };                                                                                             \
    struct ksbb_data ksbb_data_##n = {};                                                           \
    PM_DEVICE_DT_INST_DEFINE(n, ksbb_pm_action);                                                   \
    DEVICE_DT_INST_DEFINE(n, ksbb_init, PM_DEVICE_DT_INST_GET(n), &ksbb_data_##n,                  \
                          &ksbb_config_##n, POST_KERNEL,                                           \
                          CONFIG_ZMK_KSCAN_SIDEBAND_BEHAVIORS_INIT_PRIORITY, &ksbb_api);

DT_INST_FOREACH_STATUS_OKAY(KSBB_INST)
