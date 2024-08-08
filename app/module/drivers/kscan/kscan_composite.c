/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_composite

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define MATRIX_NODE_ID DT_DRV_INST(0)
#define MATRIX_ROWS DT_PROP(MATRIX_NODE_ID, rows)
#define MATRIX_COLS DT_PROP(MATRIX_NODE_ID, columns)

struct kscan_composite_child_config {
    const struct device *child;
    uint8_t row_offset;
    uint8_t column_offset;
};

#define CHILD_CONFIG(inst)                                                                         \
    {.child = DEVICE_DT_GET(DT_PHANDLE(inst, kscan)),                                              \
     .row_offset = DT_PROP(inst, row_offset),                                                      \
     .column_offset = DT_PROP(inst, column_offset)},

struct kscan_composite_config {
    const struct kscan_composite_child_config *children;
    size_t children_len;
};

struct kscan_composite_data {
    kscan_callback_t callback;

    const struct device *dev;
};

static int kscan_composite_enable_callback(const struct device *dev) {
    const struct kscan_composite_config *cfg = dev->config;

    for (int i = 0; i < cfg->children_len; i++) {
        const struct kscan_composite_child_config *child_cfg = &cfg->children[i];

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
        if (!pm_device_runtime_is_enabled(dev) && pm_device_runtime_is_enabled(child_cfg->child)) {
            pm_device_runtime_get(child_cfg->child);
        }
#elif IS_ENABLED(CONFIG_PM_DEVICE)
        pm_device_action_run(child_cfg->child, PM_DEVICE_ACTION_RESUME);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)

        kscan_enable_callback(child_cfg->child);
    }
    return 0;
}

static int kscan_composite_disable_callback(const struct device *dev) {
    const struct kscan_composite_config *cfg = dev->config;
    for (int i = 0; i < cfg->children_len; i++) {
        const struct kscan_composite_child_config *child_cfg = &cfg->children[i];

        kscan_disable_callback(child_cfg->child);

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
        if (!pm_device_runtime_is_enabled(dev) && pm_device_runtime_is_enabled(child_cfg->child)) {
            pm_device_runtime_put(child_cfg->child);
        }
#elif IS_ENABLED(CONFIG_PM_DEVICE)
        pm_device_action_run(child_cfg->child, PM_DEVICE_ACTION_SUSPEND);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)
    }
    return 0;
}

#define KSCAN_COMP_INST_DEV(n) DEVICE_DT_GET(DT_DRV_INST(n)),

static const struct device *all_instances[] = {DT_INST_FOREACH_STATUS_OKAY(KSCAN_COMP_INST_DEV)};

static void kscan_composite_child_callback(const struct device *child_dev, uint32_t row,
                                           uint32_t column, bool pressed) {
    // TODO: Ideally we can get this passed into our callback!
    for (int i = 0; i < ARRAY_SIZE(all_instances); i++) {

        const struct device *dev = all_instances[i];
        const struct kscan_composite_config *cfg = dev->config;
        struct kscan_composite_data *data = dev->data;

        for (int c = 0; c < cfg->children_len; c++) {
            const struct kscan_composite_child_config *child_cfg = &cfg->children[c];

            if (child_cfg->child != child_dev) {
                continue;
            }

            data->callback(dev, row + child_cfg->row_offset, column + child_cfg->column_offset,
                           pressed);
        }
    }
}

static int kscan_composite_configure(const struct device *dev, kscan_callback_t callback) {
    const struct kscan_composite_config *cfg = dev->config;
    struct kscan_composite_data *data = dev->data;

    if (!callback) {
        return -EINVAL;
    }

    for (int i = 0; i < cfg->children_len; i++) {
        const struct kscan_composite_child_config *child_cfg = &cfg->children[i];

        kscan_config(child_cfg->child, &kscan_composite_child_callback);
    }

    data->callback = callback;

    return 0;
}

static int kscan_composite_init(const struct device *dev) {
    struct kscan_composite_data *data = dev->data;

    data->dev = dev;

#if IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_init_suspended(dev);
#endif

    return 0;
}

static const struct kscan_driver_api mock_driver_api = {
    .config = kscan_composite_configure,
    .enable_callback = kscan_composite_enable_callback,
    .disable_callback = kscan_composite_disable_callback,
};

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int kscan_composite_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        return kscan_composite_disable_callback(dev);
    case PM_DEVICE_ACTION_RESUME:
        return kscan_composite_enable_callback(dev);
    default:
        return -ENOTSUP;
    }
}

#endif // IS_ENABLED(CONFIG_PM_DEVICE)

#define KSCAN_COMP_DEV(n)                                                                          \
    static const struct kscan_composite_child_config kscan_composite_children_##n[] = {            \
        DT_INST_FOREACH_CHILD(n, CHILD_CONFIG)};                                                   \
    static const struct kscan_composite_config kscan_composite_config_##n = {                      \
        .children = kscan_composite_children_##n,                                                  \
        .children_len = ARRAY_SIZE(kscan_composite_children_##n),                                  \
    };                                                                                             \
    static struct kscan_composite_data kscan_composite_data_##n;                                   \
    PM_DEVICE_DT_INST_DEFINE(n, kscan_composite_pm_action);                                        \
    DEVICE_DT_INST_DEFINE(n, kscan_composite_init, PM_DEVICE_DT_INST_GET(n),                       \
                          &kscan_composite_data_##n, &kscan_composite_config_##n, POST_KERNEL,     \
                          CONFIG_ZMK_KSCAN_COMPOSITE_INIT_PRIORITY, &mock_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_COMP_DEV)
