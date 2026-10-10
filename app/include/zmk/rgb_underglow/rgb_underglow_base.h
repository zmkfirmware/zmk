/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zmk/rgb_underglow/state.h>

#if !DT_HAS_CHOSEN(zmk_underglow)

#error "A zmk,underglow chosen node must be declared"

#endif

#define STRIP_CHOSEN DT_CHOSEN(zmk_underglow)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_CHOSEN, chain_length)

int zmk_rgb_ug_select_effect(int effect);
int zmk_rgb_ug_set_spd(int speed);
int zmk_rgb_ug_set_hsb(struct zmk_led_hsb color);
void zmk_rgb_ug_tick(struct k_work *work);
void zmk_rgb_ug_tools_init(const struct device *led_strip);
