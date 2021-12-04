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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef animation_api_prep_next_frame
 * @brief Callback API for generating the next animation frame
 *
 * @see animation_prep_next_frame() for argument descriptions.
 */
typedef void (*animation_api_prep_next_frame)(const struct device *dev);

/**
 * @typedef animation_api_prep_next_frame
 * @brief Callback API for generating the next animation frame
 *
 * @see animation_prep_next_frame() for argument descriptions.
 */
typedef void (*animation_api_get_pixel)(const struct device *dev,
                                        const struct animation_pixel_position *pixel_position,
                                        struct zmk_color_rgb *value);

struct animation_api {
    animation_api_prep_next_frame prep_next_frame;
    animation_api_get_pixel get_pixel;
};

/**
 * [animation_prep_next_frame description]
 * @param dev [description]
 */
static inline void animation_prep_next_frame(const struct device *dev) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    return api->prep_next_frame(dev);
}

/**
 * [animation_get_pixel description]
 * @param dev   [description]
 * @param pixel [description]
 */
static inline void animation_get_pixel(const struct device *dev,
                                       const struct animation_pixel_position *pixel_position,
                                       struct zmk_color_rgb *value) {
    const struct animation_api *api = (const struct animation_api *)dev->api;

    return api->get_pixel(dev, pixel_position, value);
}

#ifdef __cplusplus
}
#endif
