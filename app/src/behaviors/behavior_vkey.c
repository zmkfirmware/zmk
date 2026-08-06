/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_vkey

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Vendor-defined "original key": on press, set the 1-byte selector report to
// the bound id and send it; on release, set 0 and send. Runs on the central
// (no .locality -> BEHAVIOR_LOCALITY_CENTRAL), where the HID state and the USB
// endpoint live, so no split event marshaling is needed.

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d vkey id %d", event.position, binding->param1);
    zmk_hid_vkey_set((uint8_t)(binding->param1 & 0xFF));
    zmk_endpoint_send_vkey_report();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d vkey release", event.position);
    zmk_hid_vkey_clear();
    zmk_endpoint_send_vkey_report();
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_vkey_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define VKEY_INST(n)                                                                               \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_vkey_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VKEY_INST)
