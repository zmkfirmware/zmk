/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <device.h>
#include <drivers/led_strip.h>

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
 * @param hsl Color to convert
 * @param rgb Converted color
 */
void zmk_hsl_to_rgb(const struct zmk_color_hsl *hsl, struct zmk_color_rgb *rgb);

/**
 * Converts the internal RGB representation into a led_rgb struct
 * for use with led_strip drivers.
 *
 * @param rgb Color to convert
 * @param led Converted color
 */
void zmk_rgb_to_led_rgb(const struct zmk_color_rgb *rgb, struct led_rgb *led);

/**
 * Returns true if two HSL colors are the same.
 *
 * @param  a HSL color to compare
 * @param  b HSL color to compare
 * @return   True when colors share the same values
 */
bool zmk_cmp_hsl(const struct zmk_color_hsl *a, const struct zmk_color_hsl *b);

/**
 * Perform linear interpolation between HSL values of two colors
 * at a given distance (step) and store the resulting value in the given pointer.
 *
 * @param from   HSL color to interpolate
 * @param to     HSL color to interpolate
 * @param result Resulting HSL color
 * @param step   Interpolation step
 */
void zmk_interpolate_hsl(const struct zmk_color_hsl *from, const struct zmk_color_hsl *to,
                         struct zmk_color_hsl *result, float step);
