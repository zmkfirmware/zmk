/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_mouse_move

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int behavior_mouse_move_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);
    uint32_t x = (binding->param1 & 0xFFFF0000) >> 16;
    uint32_t y = binding->param1 & 0x0000FFFF;
    zmk_hid_mouse_movement_press(x, y);
    return zmk_endpoints_send_mouse_report();
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);
    uint32_t x = (binding->param1 & 0xFFFF0000) >> 16;
    uint32_t y = binding->param1 & 0x0000FFFF;
    zmk_hid_mouse_movement_release(x, y);
    return zmk_endpoints_send_mouse_report();
}

static const struct behavior_driver_api behavior_mouse_move_driver_api = {
    .binding_pressed = on_keymap_binding_pressed, .binding_released = on_keymap_binding_released};

#define KP_INST(n)                                                                                 \
    DEVICE_AND_API_INIT(behavior_mouse_move_##n, DT_INST_LABEL(n), behavior_mouse_move_init, NULL,   \
                        NULL, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                    \
                        &behavior_mouse_move_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)
