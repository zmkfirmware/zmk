/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/util.h>
#include <string.h>
#include <device.h>
#include <zmk/keys.h>
#include <zmk/behavior.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * Behavior driver API definition and system call entry points.
 *
 * (Internal use only.)
 */

typedef int (*behavior_keymap_binding_callback_t)(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event);
typedef int (*behavior_sensor_keymap_binding_callback_t)(struct zmk_behavior_binding *binding,
                                                         const struct device *sensor,
                                                         int64_t timestamp);

enum behavior_locality {
    BEHAVIOR_LOCALITY_CENTRAL,
    BEHAVIOR_LOCALITY_EVENT_SOURCE,
    BEHAVIOR_LOCALITY_GLOBAL
};

__subsystem struct behavior_driver_api {
    enum behavior_locality locality;
    behavior_keymap_binding_callback_t binding_convert_central_state_dependent_params;
    behavior_keymap_binding_callback_t binding_pressed;
    behavior_keymap_binding_callback_t binding_released;
    behavior_sensor_keymap_binding_callback_t sensor_binding_triggered;
};
/**
 * @endcond
 */

/**
 * @brief Handle the keymap binding which needs to be converted from relative "toggle" to absolute
 * "turn on"
 * @param binding Pointer to the details so of the binding
 * @param event The event that triggered use of the binding
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_keymap_binding_convert_central_state_dependent_params(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event);

static inline int z_impl_behavior_keymap_binding_convert_central_state_dependent_params(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->api;

    if (api->binding_convert_central_state_dependent_params == NULL) {
        return 0;
    }

    return api->binding_convert_central_state_dependent_params(binding, event);
}

/**
 * @brief Determine where the behavior should be run
 * @param behavior Pointer to the device structure for the driver instance.
 *
 * @retval Zero if successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_get_locality(const struct device *behavior,
                                    enum behavior_locality *locality);

static inline int z_impl_behavior_get_locality(const struct device *behavior,
                                               enum behavior_locality *locality) {
    if (behavior == NULL) {
        return -EINVAL;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)behavior->api;
    *locality = api->locality;

    return 0;
}

/**
 * @brief Handle the keymap binding being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param param1 User parameter specified at time of behavior binding.
 * @param param2 User parameter specified at time of behavior binding.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event);

static inline int z_impl_behavior_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);

    if (dev == NULL) {
        return -EINVAL;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->api;

    if (api->binding_pressed == NULL) {
        return -ENOTSUP;
    }

    return api->binding_pressed(binding, event);
}

/**
 * @brief Handle the assigned position being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param param1 User parameter specified at time of behavior assignment.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_keymap_binding_released(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event);

static inline int z_impl_behavior_keymap_binding_released(struct zmk_behavior_binding *binding,
                                                          struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);

    if (dev == NULL) {
        return -EINVAL;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->api;

    if (api->binding_released == NULL) {
        return -ENOTSUP;
    }

    return api->binding_released(binding, event);
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
__syscall int behavior_sensor_keymap_binding_triggered(struct zmk_behavior_binding *binding,
                                                       const struct device *sensor,
                                                       int64_t timestamp);

static inline int
z_impl_behavior_sensor_keymap_binding_triggered(struct zmk_behavior_binding *binding,
                                                const struct device *sensor, int64_t timestamp) {
    const struct device *dev = device_get_binding(binding->behavior_dev);

    if (dev == NULL) {
        return -EINVAL;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->api;

    if (api->sensor_binding_triggered == NULL) {
        return -ENOTSUP;
    }

    return api->sensor_binding_triggered(binding, sensor, timestamp);
}

/**
 * @}
 */

#include <syscalls/behavior.h>
