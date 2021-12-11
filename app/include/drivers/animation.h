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
    const size_t id;
    const uint8_t position_x;
    const uint8_t position_y;

    const struct device *animation;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef animation_api_prep_next_frame
 * @brief Callback run before every frame is rendered.
 *
 * @see animation_api_before_frame() for argument descriptions.
 */
typedef void (*animation_api_on_before_frame)(const struct device *dev);

/**
 * @typedef animation_api_prep_next_frame
 * @brief Callback run after every frame is rendered.
 *
 * @see animation_api_before_frame() for argument descriptions.
 */
typedef void (*animation_api_on_after_frame)(const struct device *dev);

/**
 * @typedef animation_api_prep_next_frame
 * @brief Callback API for generating the next animation frame
 *
 * @see animation_prep_next_frame() for argument descriptions.
 */
typedef void (*animation_api_render_pixel)(const struct device *dev,
                                           const struct animation_pixel *pixel,
                                           struct zmk_color_rgb *value);

struct animation_api {
    animation_api_on_before_frame on_before_frame;
    animation_api_on_after_frame on_after_frame;
    animation_api_render_pixel render_pixel;
};

static inline void animation_on_before_frame(const struct device *dev) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    if (api->on_before_frame == NULL) {
        return;
    }

    api->on_before_frame(dev);
}

static inline void animation_on_after_frame(const struct device *dev) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    if (api->on_after_frame == NULL) {
        return;
    }

    api->on_after_frame(dev);
}

static inline void animation_render_pixel(const struct device *dev,
                                          const struct animation_pixel *pixel,
                                          struct zmk_color_rgb *value) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    return api->render_pixel(dev, pixel, value);
}

#ifdef __cplusplus
}
#endif
