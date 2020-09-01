/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <zmk/keys.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * Behavior driver API definition and system call entry points.
 *
 * (Internal use only.)
 */

typedef int (*behavior_keymap_binding_callback_t)(struct device *dev, u32_t position, u32_t param1,
                                                  u32_t param2, s64_t timestamp);
typedef int (*behavior_sensor_keymap_binding_callback_t)(struct device *dev, struct device *sensor,
                                                         u32_t param1, u32_t param2);

__subsystem struct behavior_driver_api {
    behavior_keymap_binding_callback_t binding_pressed;
    behavior_keymap_binding_callback_t binding_released;
    behavior_sensor_keymap_binding_callback_t sensor_binding_triggered;
};
/**
 * @endcond
 */

/**
 * @brief Handle the keymap binding being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param param1 User parameter specified at time of behavior binding.
 * @param param2 User parameter specified at time of behavior binding.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_keymap_binding_pressed(struct device *dev, u32_t position, u32_t param1,
                                              u32_t param2, s64_t timestamp);

static inline int z_impl_behavior_keymap_binding_pressed(struct device *dev, u32_t position,
                                                         u32_t param1, u32_t param2,
                                                         s64_t timestamp) {
    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->driver_api;

    if (api->binding_pressed == NULL) {
        return -ENOTSUP;
    }

    return api->binding_pressed(dev, position, param1, param2, timestamp);
}

/**
 * @brief Handle the assigned position being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param param1 User parameter specified at time of behavior assignment.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_keymap_binding_released(struct device *dev, u32_t position, u32_t param1,
                                               u32_t param2, s64_t timestamp);

static inline int z_impl_behavior_keymap_binding_released(struct device *dev, u32_t position,
                                                          u32_t param1, u32_t param2,
                                                          s64_t timestamp) {
    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->driver_api;

    if (api->binding_released == NULL) {
        return -ENOTSUP;
    }

    return api->binding_released(dev, position, param1, param2, timestamp);
}

/**
 * @brief Handle the a sensor keymap binding being triggered
 * @param dev Pointer to the device structure for the driver instance.
 * @param sensor Pointer to the sensor device structure for the sensor driver instance.
 * @param param1 User parameter specified at time of behavior binding.
 * @param param2 User parameter specified at time of behavior binding.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_sensor_keymap_binding_triggered(struct device *dev, struct device *sensor,
                                                       u32_t param1, u32_t param2);

static inline int z_impl_behavior_sensor_keymap_binding_triggered(struct device *dev,
                                                                  struct device *sensor,
                                                                  u32_t param1, u32_t param2) {
    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->driver_api;

    if (api->sensor_binding_triggered == NULL) {
        return -ENOTSUP;
    }

    return api->sensor_binding_triggered(dev, sensor, param1, param2);
}

/**
 * @}
 */

#include <syscalls/behavior.h>
