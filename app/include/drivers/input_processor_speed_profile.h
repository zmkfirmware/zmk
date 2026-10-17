/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>

#include <drivers/input_processor.h>

struct zmk_input_processor_speed_profile_driver_api {
    struct zmk_input_processor_driver_api input_processor;
    int (*get_level)(const struct device *dev, uint8_t *level);
    int (*get_level_count)(const struct device *dev, uint8_t *count);
    int (*set_level)(const struct device *dev, uint8_t level);
};

static inline const struct zmk_input_processor_speed_profile_driver_api *
zmk_input_processor_speed_profile_api(const struct device *dev) {
    return (const struct zmk_input_processor_speed_profile_driver_api *)dev->api;
}

static inline int zmk_input_processor_speed_profile_get_level(const struct device *dev,
                                                              uint8_t *level) {
    const struct zmk_input_processor_speed_profile_driver_api *api =
        zmk_input_processor_speed_profile_api(dev);

    if (api == NULL || api->get_level == NULL) {
        return -ENOTSUP;
    }

    return api->get_level(dev, level);
}

static inline int zmk_input_processor_speed_profile_get_level_count(const struct device *dev,
                                                                    uint8_t *count) {
    const struct zmk_input_processor_speed_profile_driver_api *api =
        zmk_input_processor_speed_profile_api(dev);

    if (api == NULL || api->get_level_count == NULL) {
        return -ENOTSUP;
    }

    return api->get_level_count(dev, count);
}

static inline int zmk_input_processor_speed_profile_set_level(const struct device *dev,
                                                              uint8_t level) {
    const struct zmk_input_processor_speed_profile_driver_api *api =
        zmk_input_processor_speed_profile_api(dev);

    if (api == NULL || api->set_level == NULL) {
        return -ENOTSUP;
    }

    return api->set_level(dev, level);
}
