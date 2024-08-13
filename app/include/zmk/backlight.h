/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

int zmk_backlight_on(void);
int zmk_backlight_off(void);
int zmk_backlight_toggle(void);
bool zmk_backlight_is_on(void);

int zmk_backlight_set_brt(uint8_t brightness);
uint8_t zmk_backlight_get_brt(void);
uint8_t zmk_backlight_calc_brt(int direction);
uint8_t zmk_backlight_calc_brt_cycle(void);
