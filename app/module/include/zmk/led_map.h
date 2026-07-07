/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

int zmk_led_map_on(void);
int zmk_led_map_off(void);
int zmk_led_map_toggle(void);

int zmk_led_map_indicators_on(void);
int zmk_led_map_indicators_off(void);
int zmk_led_map_indicators_toggle(void);

int zmk_led_map_select_effect(int effect);
int zmk_led_map_cycle_effect(int direction);
int zmk_led_map_cycle_sub_effect(int direction);

int zmk_led_map_change_hue(int direction);
int zmk_led_map_change_sat(int direction);
int zmk_led_map_change_brt(int direction);
int zmk_led_map_change_spd(int direction);

int zmk_led_map_show_battery(void);
