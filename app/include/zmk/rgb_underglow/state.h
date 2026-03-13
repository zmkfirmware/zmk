/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100

struct zmk_led_hsb {
    uint16_t h;
    uint8_t s;
    uint8_t b;
};

struct rgb_underglow_state {
    struct zmk_led_hsb color;
    uint8_t animation_speed;
    uint8_t current_effect;
    uint16_t animation_step;
    bool on;
};

enum rgb_underglow_effect {
    UNDERGLOW_EFFECT_SOLID,
    UNDERGLOW_EFFECT_BREATHE,
    UNDERGLOW_EFFECT_SPECTRUM,
    UNDERGLOW_EFFECT_SWIRL,
    UNDERGLOW_EFFECT_NUMBER // Used to track number of underglow effects
};

static const struct rgb_underglow_state default_rgb_settings = (struct rgb_underglow_state){
    color : {
        h : CONFIG_ZMK_RGB_UNDERGLOW_HUE_START,
        s : CONFIG_ZMK_RGB_UNDERGLOW_SAT_START,
        b : CONFIG_ZMK_RGB_UNDERGLOW_BRT_START,
    },
    animation_speed : CONFIG_ZMK_RGB_UNDERGLOW_SPD_START,
    current_effect : CONFIG_ZMK_RGB_UNDERGLOW_EFF_START,
    animation_step : 0,
    on : IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_ON_START)
};

struct rgb_underglow_state *zmk_rgb_ug_get_state(void);
struct rgb_underglow_state *zmk_rgb_ug_get_save_state(void);
int zmk_rgb_ug_save_state(void);
int zmk_rgb_ug_state_init(void);
