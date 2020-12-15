/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

int zmk_rgb_underglow_toggle(bool saveState);
int zmk_rgb_underglow_on(bool saveState);
int zmk_rgb_underglow_off(bool saveState);
int zmk_rgb_underglow_cycle_effect(int direction);
int zmk_rgb_underglow_change_hue(int direction);
int zmk_rgb_underglow_change_sat(int direction);
int zmk_rgb_underglow_change_brt(int direction);
int zmk_rgb_underglow_change_spd(int direction);
int zmk_rgb_underglow_set_hsb(uint16_t hue, uint8_t saturation, uint8_t brightness);