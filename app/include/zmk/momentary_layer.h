/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/keymap.h>

// Locks all active momentary layers so they are not disabled when the key is
// released.
//
// Returns a set of the layers that were locked.
zmk_keymap_layers_state_t zmk_lock_active_momentary_layers();