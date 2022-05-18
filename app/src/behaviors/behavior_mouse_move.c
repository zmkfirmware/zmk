/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_mouse_move

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/mouse_move_state_changed.h>
#include <zmk/mouse.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_mouse_move_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct mouse_config *config = dev->config;
    return ZMK_EVENT_RAISE(
        zmk_mouse_move_state_changed_from_encoded(binding->param1, *config, true, event.timestamp));
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct mouse_config *config = dev->config;
    return ZMK_EVENT_RAISE(zmk_mouse_move_state_changed_from_encoded(binding->param1, *config,
                                                                     false, event.timestamp));
}

static const struct behavior_driver_api behavior_mouse_move_driver_api = {
    .binding_pressed = on_keymap_binding_pressed, .binding_released = on_keymap_binding_released};

#define KP_INST(n)                                                                                 \
    static struct mouse_config behavior_mouse_move_config_##n = {                                  \
        .delay_ms = DT_INST_PROP(n, delay_ms),                                                     \
        .time_to_max_speed_ms = DT_INST_PROP(n, time_to_max_speed_ms),                             \
        .acceleration_exponent = DT_INST_PROP(n, acceleration_exponent),                           \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, behavior_mouse_move_init, device_pm_control_nop, NULL,                \
                          &behavior_mouse_move_config_##n, APPLICATION,                            \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_mouse_move_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
