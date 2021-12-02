/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <device.h>
#include <drivers/led_strip.h>

struct animation_pixel_position {
    const uint8_t x;
    const uint8_t y;
};

struct animation_pixel {
    const struct device *animation;
    const struct animation_pixel_position position;
};

struct zmk_color_rgb {
    float r;
    float g;
    float b;
};

struct zmk_color_hsl {
    uint16_t h;
    uint8_t s;
    uint8_t l;
};

/**
 * Converts color from HSL to RGB.
 *
 * @param hsl [description]
 * @param rgb [description]
 */
void zmk_hsl_to_rgb(const struct zmk_color_hsl *hsl, struct zmk_color_rgb *rgb);

/**
 * Converts the internal RGB representation into a led_rgb struct
 * for use with led_strip drivers.
 *
 * @param rgb [description]
 * @param led [description]
 */
void zmk_rgb_to_led_rgb(const struct zmk_color_rgb *rgb, struct led_rgb *led);

/**
 * Returns true if two HSL colors are the same.
 *
 * @param  a [description]
 * @param  b [description]
 * @return   [description]
 */
bool zmk_cmp_hsl(const struct zmk_color_hsl *a, const struct zmk_color_hsl *b);

/**
 * Perform linear interpolation between HSL values of two colors
 * at a given distance (step) and store the resulting value in the given pointer.
 *
 * @param from   [description]
 * @param to     [description]
 * @param result [description]
 * @param step   [description]
 */
void zmk_interpolate_hsl(const struct zmk_color_hsl *from, const struct zmk_color_hsl *to,
                            struct zmk_color_hsl *result, float step);
