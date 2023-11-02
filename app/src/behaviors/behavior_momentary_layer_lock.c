/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_momentary_layer_lock

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/momentary_layer.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_mo_lock_init(const struct device *dev) { return 0; };

static int mo_lock_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    int fallback_layer = binding->param1;
    zmk_keymap_layers_state_t locked = zmk_lock_active_momentary_layers();
    if (!locked && fallback_layer >= 0) {
        return zmk_keymap_layer_to(fallback_layer);
    }

    return 0;
}

static const struct behavior_driver_api behavior_mo_lock_driver_api = {
    .binding_pressed = mo_lock_keymap_binding_pressed};

DEVICE_DT_INST_DEFINE(0, behavior_mo_lock_init, NULL, NULL, NULL, APPLICATION,
                      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_mo_lock_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */