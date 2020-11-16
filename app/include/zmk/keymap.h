/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

bool zmk_keymap_layer_active(u8_t layer);
int zmk_keymap_layer_activate(u8_t layer);
int zmk_keymap_layer_deactivate(u8_t layer);
int zmk_keymap_layer_toggle(u8_t layer);

int zmk_keymap_position_state_changed(u32_t position, bool pressed, s64_t timestamp);
