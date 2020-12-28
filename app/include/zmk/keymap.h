/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

typedef uint32_t zmk_keymap_layers_state_t;

uint8_t zmk_keymap_layer_default();
zmk_keymap_layers_state_t zmk_keymap_layer_state();
bool zmk_keymap_layer_active(uint8_t layer);
bool zmk_keymap_layer_active_with_state(uint8_t layer, zmk_keymap_layers_state_t state_to_test);
uint8_t zmk_keymap_highest_layer_active();
int zmk_keymap_layer_activate(uint8_t layer);
int zmk_keymap_layer_deactivate(uint8_t layer);
int zmk_keymap_layer_toggle(uint8_t layer);
int zmk_keymap_layer_to(uint8_t layer);

int zmk_keymap_position_state_changed(uint32_t position, bool pressed, int64_t timestamp);
