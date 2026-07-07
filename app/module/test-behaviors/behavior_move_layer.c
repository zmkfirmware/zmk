/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_move_layer

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if (IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)) && (DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT))

static int on_move_layer_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    int result = zmk_keymap_move_layer(binding->param1, binding->param2);

    if (result < 0) {
        LOG_ERR("Failed to move layer from index %d to index %d (err: %d)", binding->param1,
                binding->param2, result);
        return result;
    }

    LOG_DBG("Moved layer from index %d to index %d", binding->param1, binding->param2);
    return 0;
}

static int on_move_layer_binding_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    return 0;
}

static const struct behavior_driver_api behavior_move_layer_driver_api = {
    .binding_pressed = on_move_layer_binding_pressed,
    .binding_released = on_move_layer_binding_released};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_move_layer_driver_api);

#endif // IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING) AND
       // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)