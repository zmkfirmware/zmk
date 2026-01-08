/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_add_layer

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if (IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)) && (DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT))

static int on_add_layer_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    int new_layer = zmk_keymap_add_layer();
    if (new_layer >= 0) {
        LOG_DBG("Added layer %d", new_layer);
        return 0;
    }
    switch (new_layer) {
    case -ENOSPC:
        LOG_ERR("No more layers can be added. Out of memory.");
        return -ENOSPC;
    default:
        LOG_ERR("Unknown error adding layer: %d", new_layer);
        return new_layer;
    }
}

static int on_add_layer_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    return 0;
}

static const struct behavior_driver_api behavior_add_layer_driver_api = {
    .binding_pressed = on_add_layer_binding_pressed,
    .binding_released = on_add_layer_binding_released};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_add_layer_driver_api);

#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING) AND
       // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)