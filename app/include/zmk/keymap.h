/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

typedef u32_t zmk_keymap_layers_state;

u8_t zmk_keymap_layer_default();
zmk_keymap_layers_state zmk_keymap_layer_state();
bool zmk_keymap_layer_active(u8_t layer);
u8_t zmk_keymap_highest_layer_active();
int zmk_keymap_layer_activate(u8_t layer);
int zmk_keymap_layer_deactivate(u8_t layer);
int zmk_keymap_layer_toggle(u8_t layer);

int zmk_keymap_position_state_changed(u32_t position, bool pressed, s64_t timestamp);
