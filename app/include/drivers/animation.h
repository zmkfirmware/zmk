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
 * @typedef animation_start
 * @brief Callback API for starting an animation.
 *
 * @see animation_start() for argument descriptions.
 */
typedef void (*animation_api_start)(const struct device *dev);

/**
 * @typedef animation_stop
 * @brief Callback API for stopping an animation.
 *
 * @see animation_stop() for argument descriptions.
 */
typedef void (*animation_api_stop)(const struct device *dev);

/**
 * @typedef animation_render_frame
 * @brief Callback API for generating the next animation frame.
 *
 * @see animation_render_frame() for argument descriptions.
 */
typedef void (*animation_api_render_frame)(const struct device *dev, struct animation_pixel *pixels,
                                           size_t num_pixels);

struct animation_api {
    animation_api_start on_start;
    animation_api_stop on_stop;
    animation_api_render_frame render_frame;
};

static inline void animation_start(const struct device *dev) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    return api->on_start(dev);
}

static inline void animation_stop(const struct device *dev) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    return api->on_stop(dev);
}

static inline void animation_render_frame(const struct device *dev, struct animation_pixel *pixels,
                                          size_t num_pixels) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    return api->render_frame(dev, pixels, num_pixels);
}

#ifdef __cplusplus
}
#endif
