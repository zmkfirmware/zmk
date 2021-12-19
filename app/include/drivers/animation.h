/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <device.h>
#include <drivers/led_strip.h>

#include <zmk/animation.h>

/**
 * @file
 * #brief Public API for controlling animations.
 *
 * This library abstracts the implementation details
 * for various types of 2D animations.
 */

struct animation_pixel {
    const uint8_t position_x;
    const uint8_t position_y;

    struct zmk_color_rgb value;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef animation_render_frame
 * @brief Callback API for generating the next animation frame
 *
 * @see animation_render_frame() for argument descriptions.
 */
typedef void (*animation_api_render_frame)(const struct device *dev, struct animation_pixel *pixels,
                                           size_t num_pixels);

struct animation_api {
    animation_api_render_frame render_frame;
};

static inline void animation_render_frame(const struct device *dev, struct animation_pixel *pixels,
                                          size_t num_pixels) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    return api->render_frame(dev, pixels, num_pixels);
}

#ifdef __cplusplus
}
#endif
