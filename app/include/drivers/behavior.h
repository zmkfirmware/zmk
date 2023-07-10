/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/device.h>
#include <zmk/keys.h>
#include <zmk/sensors.h>
#include <zmk/behavior.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * Behavior driver API definition and system call entry points.
 *
 * (Internal use only.)
 */

enum behavior_sensor_binding_process_mode {
    BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_TRIGGER,
    BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_DISCARD,
};

typedef int (*behavior_keymap_binding_callback_t)(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event);
typedef int (*behavior_sensor_keymap_binding_process_callback_t)(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
    enum behavior_sensor_binding_process_mode mode);
typedef int (*behavior_sensor_keymap_binding_accept_data_callback_t)(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
    const struct zmk_sensor_config *sensor_config, size_t channel_data_size,
    const struct zmk_sensor_channel_data channel_data[channel_data_size]);

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
    behavior_sensor_keymap_binding_accept_data_callback_t sensor_binding_accept_data;
    behavior_sensor_keymap_binding_process_callback_t sensor_binding_process;
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
 * @brief Handle the a sensor keymap binding processing any incoming data from the sensor
 * @param binding Sensor keymap binding which was triggered.
 * @param sensor Pointer to the sensor device structure for the sensor driver instance.
 * @param virtual_key_position ZMK_KEYMAP_LEN + sensor number
 * @param timestamp Time at which the binding was triggered.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_sensor_keymap_binding_accept_data(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
    const struct zmk_sensor_config *sensor_config, size_t channel_data_size,
    const struct zmk_sensor_channel_data *channel_data);

static inline int z_impl_behavior_sensor_keymap_binding_accept_data(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
    const struct zmk_sensor_config *sensor_config, size_t channel_data_size,
    const struct zmk_sensor_channel_data *channel_data) {
    const struct device *dev = device_get_binding(binding->behavior_dev);

    if (dev == NULL) {
        return -EINVAL;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->api;

    if (api->sensor_binding_accept_data == NULL) {
        return -ENOTSUP;
    }

    return api->sensor_binding_accept_data(binding, event, sensor_config, channel_data_size,
                                           channel_data);
}

/**
 * @brief Handle the keymap sensor binding being triggered after updating any local data
 * @param dev Pointer to the device structure for the driver instance.
 * @param param1 User parameter specified at time of behavior binding.
 * @param param2 User parameter specified at time of behavior binding.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
// clang-format off
__syscall int behavior_sensor_keymap_binding_process(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event event,
    enum behavior_sensor_binding_process_mode mode);
// clang-format on

static inline int
z_impl_behavior_sensor_keymap_binding_process(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event,
                                              enum behavior_sensor_binding_process_mode mode) {
    const struct device *dev = device_get_binding(binding->behavior_dev);

    if (dev == NULL) {
        return -EINVAL;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->api;

    if (api->sensor_binding_process == NULL) {
        return -ENOTSUP;
    }

    return api->sensor_binding_process(binding, event, mode);
}

/**
 * @}
 */

#include <syscalls/behavior.h>
