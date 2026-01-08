/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_remove_layer

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if (IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)) && (DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT))

static int on_remove_layer_binding_pressed(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    int result = zmk_keymap_remove_layer(binding->param1);
    if (result >= 0) {
        LOG_DBG("Removed layer at index %d", binding->param1);
        return 0;
    }
    switch (result) {
    case -EINVAL:
        LOG_ERR("Layer at index %d not found", binding->param1);
        return -EINVAL;
    default:
        LOG_DBG("Unknown error removing layer at index %d: %d", binding->param1, result);
        return result;
    }
}

static int on_remove_layer_binding_released(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    return 0;
}

static const struct behavior_driver_api behavior_remove_layer_driver_api = {
    .binding_pressed = on_remove_layer_binding_pressed,
    .binding_released = on_remove_layer_binding_released};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_remove_layer_driver_api);
#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING) AND
       // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)