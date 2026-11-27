/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_inject_input_speed_setting

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <drivers/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_inject_input_speed_setting_config {
    const struct device *processor;
};

static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_inject_input_speed_setting_config *config = dev->config;

    ARG_UNUSED(event);

#if IS_ENABLED(CONFIG_SETTINGS_RUNTIME)
    uint8_t level = (uint8_t)binding->param1;
    char path[SETTINGS_MAX_NAME_LEN + 1];
    int len = snprintf(path, sizeof(path), "input_speed/%s", config->processor->name);

    if (len < 0 || (size_t)len >= sizeof(path)) {
        return -ENAMETOOLONG;
    }

    int err = settings_runtime_set(path, &level, sizeof(level));
    if (err != 0) {
        LOG_ERR("Input speed setting inject failed: level=%u err=%d", level, err);
        return err;
    }

    LOG_DBG("Input speed setting injected: level=%u", level);
    return 0;
#else
    LOG_ERR("Input speed setting inject requires CONFIG_SETTINGS_RUNTIME");
    return -ENOTSUP;
#endif
}

static int on_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_inject_input_speed_setting_driver_api = {
    .binding_pressed = on_binding_pressed,
    .binding_released = on_binding_released,
};

#define INJECT_INPUT_SPEED_SETTING_DEFINE(inst)                                                    \
    static const struct behavior_inject_input_speed_setting_config config_##inst = {               \
        .processor = DEVICE_DT_GET(DT_INST_PHANDLE(inst, processor)),                              \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(inst, NULL, NULL, NULL, &config_##inst, POST_KERNEL,                   \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_inject_input_speed_setting_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INJECT_INPUT_SPEED_SETTING_DEFINE)

#endif
