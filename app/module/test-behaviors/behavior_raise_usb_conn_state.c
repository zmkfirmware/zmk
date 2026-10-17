/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_raise_usb_conn_state

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_raise_usb_conn_state_binding_pressed(struct zmk_behavior_binding *binding,
                                                   struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    return raise_zmk_usb_conn_state_changed(
        (struct zmk_usb_conn_state_changed){.conn_state = binding->param1});
}

static int on_raise_usb_conn_state_binding_released(struct zmk_behavior_binding *binding,
                                                    struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    return 0;
}

static const struct behavior_driver_api behavior_raise_usb_conn_state_driver_api = {
    .binding_pressed = on_raise_usb_conn_state_binding_pressed,
    .binding_released = on_raise_usb_conn_state_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_raise_usb_conn_state_driver_api);

#endif
