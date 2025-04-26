/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

bool zmk_is_layers_mask_locked(uint32_t layers_mask);

bool zmk_is_layer_locked(uint8_t layer);

void zmk_layer_lock_toggle(uint32_t layers_mask);
