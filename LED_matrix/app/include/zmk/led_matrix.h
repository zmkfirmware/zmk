/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zmk/keymap.h>

enum zmk_led_matrix_render_source {
    ZMK_LED_MATRIX_RENDER_SOURCE_OFF = 0,
    ZMK_LED_MATRIX_RENDER_SOURCE_PROFILE,
    ZMK_LED_MATRIX_RENDER_SOURCE_INDEPENDENT,
    ZMK_LED_MATRIX_RENDER_SOURCE_LAYER_COLOR,
    ZMK_LED_MATRIX_RENDER_SOURCE_STATUS_OVERLAY,
};

struct zmk_led_matrix_state {
    bool enabled;
    uint8_t brightness;
    enum zmk_led_matrix_render_source effective_source;
    uint8_t selected_profile_id;
    const char *selected_profile_name;
    const char *selected_renderer_name;
    uint8_t effective_status_sources_mask;
    zmk_keymap_layer_id_t effective_layer_color_layer_id;
};

int zmk_led_matrix_on(void);
int zmk_led_matrix_off(void);
int zmk_led_matrix_toggle(void);
bool zmk_led_matrix_is_on(void);

int zmk_led_matrix_set_brt(uint8_t brightness);
uint8_t zmk_led_matrix_get_brt(void);
uint8_t zmk_led_matrix_calc_brt(int direction);

int zmk_led_matrix_set_spd(uint8_t speed);
uint8_t zmk_led_matrix_get_spd(void);
uint8_t zmk_led_matrix_calc_spd(int direction);

int zmk_led_matrix_set_hue_index(uint8_t index);
uint8_t zmk_led_matrix_get_hue_index(void);
uint8_t zmk_led_matrix_calc_hue_index(int direction);

int zmk_led_matrix_set_profile(uint8_t profile_id);
uint8_t zmk_led_matrix_get_profile(void);
uint8_t zmk_led_matrix_calc_profile(int direction);

int zmk_led_matrix_set_independent_profile(uint8_t child_id, uint8_t profile_id);
uint8_t zmk_led_matrix_get_independent_profile(uint8_t child_id);
uint8_t zmk_led_matrix_calc_independent_profile(uint8_t child_id, int direction);

int zmk_led_matrix_get_state(struct zmk_led_matrix_state *out);

int zmk_led_matrix_show_status(uint8_t sources_mask);
