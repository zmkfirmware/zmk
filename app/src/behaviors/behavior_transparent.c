/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_transparent

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/keymap.h>

#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_keymap_binding_trigger(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    // Avoid uint8_t overflow resulting in an infinite loop
    if (LAYER_ID_TO_INDEX(event.layer) == 0) {
        return 0;
    }
    return zmk_keymap_raise_binding_event_at_layer_index(LAYER_ID_TO_INDEX(event.layer) - 1,
                                                         event.source, event.position, event.type,
                                                         event.timestamp);
}

static const struct behavior_driver_api behavior_transparent_driver_api = {
    .binding_pressed = on_keymap_binding_trigger,
    .binding_released = on_keymap_binding_trigger,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_transparent_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
