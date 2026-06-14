/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_composite

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define MATRIX_NODE_ID DT_DRV_INST(0)
#define MATRIX_ROWS DT_PROP(MATRIX_NODE_ID, rows)
#define MATRIX_COLS DT_PROP(MATRIX_NODE_ID, columns)

struct input_composite_child_ref {
    const struct device *parent;
    uint8_t child_idx;
};

struct input_composite_child_config {
    const struct device *child;
    uint8_t row_offset;
    uint8_t column_offset;
};

struct input_composite_child_data {
    uint8_t row;
    uint8_t col;
    bool pressed;
};

#define CHILD_CONFIG(inst)                                                                         \
    {.child = DEVICE_DT_GET(DT_PHANDLE(inst, input)),                                              \
     .row_offset = DT_PROP(inst, row_offset),                                                      \
     .column_offset = DT_PROP_OR(inst, col_offset, DT_PROP(inst, column_offset))},

struct input_composite_config {
    const struct input_composite_child_config *children;
    struct input_composite_child_data *child_data;
    size_t children_len;
};

static int input_composite_enable_callback(const struct device *dev) {
    const struct input_composite_config *cfg = dev->config;

    for (int i = 0; i < cfg->children_len; i++) {
        const struct input_composite_child_config *child_cfg = &cfg->children[i];

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || IS_ENABLED(CONFIG_PM_DEVICE)
        if (pm_device_wakeup_is_enabled(dev) && pm_device_wakeup_is_capable(child_cfg->child) &&
            !pm_device_wakeup_enable(child_cfg->child, true)) {
            LOG_ERR("Failed to enable wakeup for %s", child_cfg->child->name);
        }
#endif // IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || IS_ENABLED(CONFIG_PM_DEVICE)

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
        if (!pm_device_runtime_is_enabled(dev) && pm_device_runtime_is_enabled(child_cfg->child)) {
            pm_device_runtime_get(child_cfg->child);
        }
#elif IS_ENABLED(CONFIG_PM_DEVICE)
        pm_device_action_run(child_cfg->child, PM_DEVICE_ACTION_RESUME);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)
    }
    return 0;
}

static int input_composite_disable_callback(const struct device *dev) {
    const struct input_composite_config *cfg = dev->config;
    for (int i = 0; i < cfg->children_len; i++) {
        const struct input_composite_child_config *child_cfg = &cfg->children[i];

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || IS_ENABLED(CONFIG_PM_DEVICE)
        if (pm_device_wakeup_is_capable(child_cfg->child) &&
            pm_device_wakeup_is_enabled(child_cfg->child) &&
            !pm_device_wakeup_enable(child_cfg->child, false)) {
            LOG_ERR("Failed to disable wakeup for %s", child_cfg->child->name);
        }
#endif // IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || IS_ENABLED(CONFIG_PM_DEVICE)

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

static int input_composite_init(const struct device *dev) {

#if IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_init_suspended(dev);
#endif

    return 0;
}

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int input_composite_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        return input_composite_disable_callback(dev);
    case PM_DEVICE_ACTION_RESUME:
        return input_composite_enable_callback(dev);
    default:
        return -ENOTSUP;
    }
}

#endif // IS_ENABLED(CONFIG_PM_DEVICE)

static void input_composite_input_event_cb(struct input_event *evt, void *user_data) {
    const struct input_composite_child_ref *ref =
        (const struct input_composite_child_ref *)user_data;

    const struct device *dev = ref->parent;
    const struct input_composite_config *cfg = dev->config;

    const struct input_composite_child_config *child_cfg = &cfg->children[ref->child_idx];
    struct input_composite_child_data *child_data = &cfg->child_data[ref->child_idx];

    switch (evt->type) {
    case INPUT_EV_ABS:
        switch (evt->code) {
        case INPUT_ABS_X:
            child_data->col = evt->value;
            break;
        case INPUT_ABS_Y:
            child_data->row = evt->value;
            break;
        default:
            LOG_WRN("Unknown abs code");
            return;
        }
        break;
    case INPUT_EV_KEY:
        switch (evt->code) {
        case INPUT_BTN_TOUCH:
            child_data->pressed = evt->value;
            break;
        default:
            LOG_WRN("Unknown key code");
            return;
        }
        break;
    default:
        LOG_WRN("Unknown type");
        return;
    }

    if (evt->sync) {
        input_report_abs(dev, INPUT_ABS_X, child_data->col + child_cfg->column_offset, false,
                         K_FOREVER);
        input_report_abs(dev, INPUT_ABS_Y, child_data->row + child_cfg->row_offset, false,
                         K_FOREVER);
        input_report_key(dev, INPUT_BTN_TOUCH, child_data->pressed, true, K_FOREVER);
    }
}

#define CHILD_INPUT_LISTENER(node)                                                                 \
    static const struct input_composite_child_ref _CONCAT(ref, node) = {                           \
        .parent = DEVICE_DT_GET(DT_PARENT(node)),                                                  \
        .child_idx = DT_NODE_CHILD_IDX(node),                                                      \
    };                                                                                             \
    INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(DT_PHANDLE(node, input)),                            \
                                input_composite_input_event_cb, (void *)&(_CONCAT(ref, node)),     \
                                input_composite_input_cb_##node);

#define INPUT_COMP_DEV(n)                                                                          \
    DT_INST_FOREACH_CHILD(n, CHILD_INPUT_LISTENER)                                                 \
    static const struct input_composite_child_config input_composite_children_##n[] = {            \
        DT_INST_FOREACH_CHILD(n, CHILD_CONFIG)};                                                   \
    static struct input_composite_child_data input_composite_child_data_##n[DT_INST_CHILD_NUM(n)]; \
    static const struct input_composite_config input_composite_config_##n = {                      \
        .children = input_composite_children_##n,                                                  \
        .child_data = input_composite_child_data_##n,                                              \
        .children_len = ARRAY_SIZE(input_composite_children_##n),                                  \
    };                                                                                             \
    PM_DEVICE_DT_INST_DEFINE(n, input_composite_pm_action);                                        \
    DEVICE_DT_INST_DEFINE(n, input_composite_init, PM_DEVICE_DT_INST_GET(n), NULL,                 \
                          &input_composite_config_##n, POST_KERNEL,                                \
                          CONFIG_ZMK_INPUT_COMPOSITE_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_COMP_DEV)
