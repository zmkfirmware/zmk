/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <zmk/layer_lock_state.h>
#include <zephyr/kernel.h>

static uint8_t locked_layers_mask = 0;

bool zmk_is_layers_mask_locked(uint32_t layers_mask) {
    return (locked_layers_mask & layers_mask) == layers_mask;
}

bool zmk_is_layer_locked(uint8_t layer) { return (locked_layers_mask & BIT(layer)) != 0; }

void zmk_layer_lock_toggle(uint32_t layers_mask) { locked_layers_mask ^= layers_mask; }
