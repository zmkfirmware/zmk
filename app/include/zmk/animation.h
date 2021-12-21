/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <device.h>
#include <drivers/led_strip.h>

#define ZMK_ANIMATION_BLENDING_MODE_NORMAL 0
#define ZMK_ANIMATION_BLENDING_MODE_MULTIPLY 1
#define ZMK_ANIMATION_BLENDING_MODE_LIGHTEN 2
#define ZMK_ANIMATION_BLENDING_MODE_DARKEN 3
#define ZMK_ANIMATION_BLENDING_MODE_SCREEN 4
#define ZMK_ANIMATION_BLENDING_MODE_SUBTRACT 5

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

#if DT_NODE_HAS_PROP(DT_INST(0, animation), key_position)
size_t zmk_animation_get_pixel_by_key_position(size_t key_position);
#else
static inline size_t zmk_animation_get_pixel_by_key_position(size_t key_position) {
    return key_position;
}
#endif

#if defined(CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE) && (CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE == 1)
uint8_t zmk_animation_get_pixel_distance(size_t pixel_idx, size_t other_pixel_idx);
#endif

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

void zmk_animation_request_frames(uint32_t frames);

struct zmk_color_rgb __zmk_apply_blending_mode(struct zmk_color_rgb base_value,
                                               struct zmk_color_rgb blend_value, uint8_t mode);

static inline struct zmk_color_rgb zmk_apply_blending_mode(struct zmk_color_rgb base_value,
                                                           struct zmk_color_rgb blend_value,
                                                           uint8_t mode) {
    if (mode == ZMK_ANIMATION_BLENDING_MODE_NORMAL) {
        return blend_value;
    }

    return __zmk_apply_blending_mode(base_value, blend_value, mode);
}
