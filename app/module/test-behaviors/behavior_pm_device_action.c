/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_pm_device_action

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/pm/device.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_pm_device_action_config {
    const struct device *target;
};

static int on_behavior_pm_device_action_pressed(struct zmk_behavior_binding *binding,
                                                struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_pm_device_action_config *config = dev->config;

    ARG_UNUSED(event);

    switch (binding->param1) {
    case 0:
        return pm_device_action_run(config->target, PM_DEVICE_ACTION_SUSPEND);
    case 1:
        return pm_device_action_run(config->target, PM_DEVICE_ACTION_RESUME);
    default:
        return -EINVAL;
    }
}

static int on_behavior_pm_device_action_released(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    return 0;
}

static const struct behavior_driver_api behavior_pm_device_action_driver_api = {
    .binding_pressed = on_behavior_pm_device_action_pressed,
    .binding_released = on_behavior_pm_device_action_released,
};

#define PM_DEVICE_ACTION_INST(n)                                                                   \
    static const struct behavior_pm_device_action_config behavior_pm_device_action_config_##n = {  \
        .target = DEVICE_DT_GET(DT_INST_PHANDLE(n, device)),                                       \
    };                                                                                             \
                                                                                                   \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_pm_device_action_config_##n,            \
                            POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,                         \
                            &behavior_pm_device_action_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PM_DEVICE_ACTION_INST)

#endif
