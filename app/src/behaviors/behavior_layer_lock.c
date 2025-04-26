/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/layer_lock_state.h>

#define DT_DRV_COMPAT zmk_behavior_layer_lock

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define ZMK_BEHAVIOR_LAYER_MOMENTARY "ZMK_BEHAVIOR_LAYER_MOMENTARY"

static uint32_t find_active_layers_mask_for_position(uint32_t position) {
    uint32_t mask = 0;
    uint32_t state = zmk_keymap_layer_state();

    for (int8_t i = 31; i >= 0; i--) {
        if (!(state & BIT(i))) {
            continue;
        }

        const struct zmk_behavior_binding *binding =
            zmk_keymap_get_layer_binding_at_idx(i, position);

        if (!binding) {
            continue;
        }

        if (*(binding->behavior_dev) == ZMK_BEHAVIOR_TRANSPARENT) {
            continue;
        }

        mask |= BIT(i);
    }

    return mask;
}

static int layer_lock_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    uint8_t active_layers_mask = find_active_layers_mask_for_position(event.position);
    zmk_layer_lock_toggle(active_layers_mask);

    if (zmk_is_layers_mask_locked(active_layers_mask)) {
        LOG_DBG("locked layers mask %d", active_layers_mask);
    } else {
        LOG_DBG("unlocked layers mask %d", active_layers_mask);
        uint32_t state = zmk_keymap_layer_state();
        for (int8_t i = 31; i > 0; i--) {
            if (state & BIT(i)) {
                zmk_keymap_layer_deactivate(i);
            }
        }
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int layer_lock_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api layer_lock_driver_api = {
    .binding_pressed = layer_lock_pressed,
    .binding_released = layer_lock_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &layer_lock_driver_api);

#endif
