/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_sideband_behaviors

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/input/input.h>
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
    const struct device *input;
    bool auto_enable;
    struct ksbb_entry *entries;
    size_t entries_len;
};

struct ksbb_data {
    uint8_t row;
    uint8_t col;
    bool state;
    bool enabled;
};

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

static void ksbb_input_event_cb(struct input_event *evt, void *user_data) {
    const struct device *ksbb = (const struct device *)user_data;
    struct ksbb_data *data = ksbb->data;

    __ASSERT(evt != NULL, "Expecting an input event");

    switch (evt->type) {
    case INPUT_EV_ABS:
        switch (evt->code) {
        case INPUT_ABS_X:
            data->col = evt->value;
            break;
        case INPUT_ABS_Y:
            data->row = evt->value;
            break;
        default:
            LOG_WRN("Unknown abs code");
            return;
        }
        break;
    case INPUT_EV_KEY:
        switch (evt->code) {
        case INPUT_BTN_TOUCH:
            data->state = evt->value;
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
        struct ksbb_entry *entry = find_sideband_behavior(ksbb, data->row, data->col);
        if (entry) {
            struct zmk_behavior_binding_event event = {.position = INT32_MAX,
                                                       .timestamp = k_uptime_get()};
    
            if (data->state) {
                behavior_keymap_binding_pressed(&entry->binding, event);
            } else {
                behavior_keymap_binding_released(&entry->binding, event);
            }
        }
    }

    if (data->enabled) {
        input_report(ksbb, evt->type, evt->code, evt->value, evt->sync, K_NO_WAIT);
    }
}

static int ksbb_enable(const struct device *dev) {
    struct ksbb_data *data = dev->data;
    const struct ksbb_config *config = dev->config;
    data->enabled = true;

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    if (!pm_device_runtime_is_enabled(dev) && pm_device_runtime_is_enabled(config->input)) {
        pm_device_runtime_get(config->input);
    }
#elif IS_ENABLED(CONFIG_PM_DEVICE)
    if (pm_device_wakeup_is_capable(config->input)) {
        pm_device_wakeup_enable(config->input, true);
    }
    pm_device_action_run(config->input, PM_DEVICE_ACTION_RESUME);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)

    return 0;
}

static int ksbb_disable(const struct device *dev) {
    struct ksbb_data *data = dev->data;
    const struct ksbb_config *config = dev->config;
    data->enabled = false;

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    if (!pm_device_runtime_is_enabled(dev) && pm_device_runtime_is_enabled(config->input)) {
        pm_device_runtime_put(config->input);
    }
#elif IS_ENABLED(CONFIG_PM_DEVICE)
    if (pm_device_wakeup_is_capable(config->input) && !pm_device_wakeup_is_enabled(dev) &&
        pm_device_wakeup_is_enabled(config->input)) {
        pm_device_wakeup_enable(config->input, false);
    }
    pm_device_action_run(config->input, PM_DEVICE_ACTION_SUSPEND);
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

    if (!device_is_ready(config->input)) {
        LOG_ERR("input %s is not ready", config->input->name);
        return -ENODEV;
    }

#if IS_ENABLED(CONFIG_PM_DEVICE)
    if (!config->auto_enable) {
        pm_device_init_suspended(dev);
    }
#endif

    return 0;
}

#define ENTRY(e)                                                                                   \
    {                                                                                              \
        .row = DT_PROP(e, row),                                                                    \
        .column = DT_PROP(e, column),                                                              \
        .binding = ZMK_KEYMAP_EXTRACT_BINDING(0, e),                                               \
    }

#define KSBB_INST(n)                                                                               \
    COND_CODE_1(DT_INST_PROP_OR(n, auto_enable, false), (static int ksbb_auto_enable_##n(void) {   \
                    const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(n));                      \
                    COND_CODE_1(IS_ENABLED(CONFIG_PM_DEVICE),                                      \
                                (ksbb_pm_action(dev, PM_DEVICE_ACTION_RESUME);),                   \
                                ()); \
                    return 0;                                                                      \
                } SYS_INIT(ksbb_auto_enable_##n, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);), \
                ())                                                                                \
    static struct ksbb_entry entries_##n[] = {                                                     \
        DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, ENTRY, (, ))};                                    \
    const struct ksbb_config ksbb_config_##n = {                                                   \
        .input = DEVICE_DT_GET(DT_INST_PHANDLE(n, input)),                                         \
        .auto_enable = DT_INST_PROP_OR(n, auto_enable, false),                                     \
        .entries = entries_##n,                                                                    \
        .entries_len = ARRAY_SIZE(entries_##n),                                                    \
    };                                                                                             \
    INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(DT_INST_PHANDLE(n, input)), \
                  ksbb_input_event_cb, \
                  (void *)DEVICE_DT_GET(DT_DRV_INST(n)), zmk_input_sideband_behavior_cb_##n); \
    struct ksbb_data ksbb_data_##n = {};                                                           \
    PM_DEVICE_DT_INST_DEFINE(n, ksbb_pm_action);                                                   \
    DEVICE_DT_INST_DEFINE(n, ksbb_init, PM_DEVICE_DT_INST_GET(n), &ksbb_data_##n,                  \
                          &ksbb_config_##n, POST_KERNEL,                                           \
                          CONFIG_ZMK_INPUT_SIDEBAND_BEHAVIORS_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(KSBB_INST)
