/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

int zmk_backlight_set_on(bool on);
bool zmk_backlight_is_on();

int zmk_backlight_set_brt(int brt);
int zmk_backlight_get_brt();

int zmk_backlight_toggle();
int zmk_backlight_on();
int zmk_backlight_off();
int zmk_backlight_inc();
int zmk_backlight_dec();
