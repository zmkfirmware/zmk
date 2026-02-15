/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_mouse_speed

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <dt-bindings/zmk/pointing.h>
#include <zmk/pointing/behavior_input_two_axis.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_mouse_speed_config {
    const struct device *target;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_mouse_speed_config *cfg = dev->config;

    LOG_DBG("position %d speed 0x%08X", event.position, binding->param1);

    int16_t mul = SPEED_MUL_DECODE(binding->param1);
    int16_t div = SPEED_DIV_DECODE(binding->param1);

    return behavior_input_two_axis_set_speed_multiplier(cfg->target, mul, div);
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_mouse_speed_config *cfg = dev->config;

    LOG_DBG("position %d speed 0x%08X", event.position, binding->param1);

    return behavior_input_two_axis_set_speed_multiplier(cfg->target, 1, 1);
}

static const struct behavior_driver_api behavior_mouse_speed_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define MMS_INST(n)                                                                                \
    static const struct behavior_mouse_speed_config behavior_mouse_speed_config_##n = {            \
        .target = DEVICE_DT_GET(DT_INST_PHANDLE(n, target)),                                       \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_mouse_speed_config_##n, POST_KERNEL,    \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_mouse_speed_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MMS_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
