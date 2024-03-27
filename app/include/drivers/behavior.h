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

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

enum behavior_parameter_standard_domain {
    BEHAVIOR_PARAMETER_STANDARD_DOMAIN_NULL = 0,
    BEHAVIOR_PARAMETER_STANDARD_DOMAIN_HID_USAGE = 1,
    BEHAVIOR_PARAMETER_STANDARD_DOMAIN_LAYER_INDEX = 2,
    BEHAVIOR_PARAMETER_STANDARD_DOMAIN_HSV = 3,
};

struct behavior_parameter_value_metadata {
    char *friendly_name;
    uint8_t position;

    union {
        uint32_t value;
        struct {
            uint16_t min;
            uint16_t max;
        } range;

        enum behavior_parameter_standard_domain standard;
    };

    enum {
        BEHAVIOR_PARAMETER_VALUE_METADATA_TYPE_VALUE = 0,
        BEHAVIOR_PARAMETER_VALUE_METADATA_TYPE_RANGE = 1,
        BEHAVIOR_PARAMETER_VALUE_METADATA_TYPE_STANDARD = 2,
    } type;
};

struct behavior_parameter_metadata_custom_set {
    size_t values_len;
    const struct behavior_parameter_value_metadata *values;
};

struct behavior_parameter_metadata_custom {
    size_t sets_len;
    struct behavior_parameter_metadata_custom_set sets[];
};

struct behavior_parameter_metadata {
    union {
        struct {
            uint16_t param1;
            uint16_t param2;
        } standard;

        const struct behavior_parameter_metadata_custom *custom;
    };

    enum {
        BEHAVIOR_PARAMETER_METADATA_STANDARD = 0,
        BEHAVIOR_PARAMETER_METADATA_CUSTOM = 1,
    } type;
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

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
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
typedef int (*behavior_get_parameter_domains_t)(const struct device *behavior,
                                                struct behavior_parameter_metadata *param_metadata);
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

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
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    behavior_get_parameter_domains_t get_parameter_domains;
    const struct behavior_parameter_metadata_custom *custom_parameters;
    const enum behavior_parameter_standard_domain param1_standard_domain;
    const enum behavior_parameter_standard_domain param2_standard_domain;
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};
/**
 * @endcond
 */

struct zmk_behavior_ref {
    const struct device *device;
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    const char *friendly_name;
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define ZMK_BEHAVIOR_REF_DT_NAME(node_id) _CONCAT(zmk_behavior_, DEVICE_DT_NAME_GET(node_id))

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

#define ZMK_BEHAVIOR_REF_INITIALIZER(_dev, _friendly_name)                                         \
    { .device = _dev, .friendly_name = _friendly_name, }

#define ZMK_BEHAVIOR_REF_DEFINE(name, ...)                                                         \
    static const STRUCT_SECTION_ITERABLE(zmk_behavior_ref, name) =                                 \
        ZMK_BEHAVIOR_REF_INITIALIZER(__VA_ARGS__)

#define ZMK_BEHAVIOR_REF_DT_DEFINE(node_id)                                                        \
    ZMK_BEHAVIOR_REF_DEFINE(ZMK_BEHAVIOR_REF_DT_NAME(node_id), DEVICE_DT_GET(node_id),             \
                            DT_PROP_OR(node_id, friendly_name, DEVICE_DT_NAME(node_id)))

#else

#define ZMK_BEHAVIOR_REF_INITIALIZER(_dev)                                                         \
    { .device = _dev, }

#define ZMK_BEHAVIOR_REF_DEFINE(name, ...)                                                         \
    static const STRUCT_SECTION_ITERABLE(zmk_behavior_ref, name) =                                 \
        ZMK_BEHAVIOR_REF_INITIALIZER(__VA_ARGS__)

#define ZMK_BEHAVIOR_REF_DT_DEFINE(node_id)                                                        \
    ZMK_BEHAVIOR_REF_DEFINE(ZMK_BEHAVIOR_REF_DT_NAME(node_id), DEVICE_DT_GET(node_id))

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

/**
 * Registers @p node_id as a behavior.
 */
#define BEHAVIOR_DEFINE(node_id) ZMK_BEHAVIOR_REF_DT_DEFINE(node_id)

/**
 * @brief Like DEVICE_DT_DEFINE(), but also registers the device as a behavior.
 *
 * @param node_id The devicetree node identifier.
 * @param ... Other parameters as expected by DEVICE_DT_DEFINE.
 */
#define BEHAVIOR_DT_DEFINE(node_id, ...)                                                           \
    DEVICE_DT_DEFINE(node_id, __VA_ARGS__);                                                        \
    BEHAVIOR_DEFINE(node_id)

/**
 * @brief Like DEVICE_DT_INST_DEFINE(), but also registers the device as a behavior.
 *
 * @param inst Instance number.
 * @param ... Other parameters as expected by DEVICE_DT_DEFINE.
 */
#define BEHAVIOR_DT_INST_DEFINE(inst, ...)                                                         \
    DEVICE_DT_INST_DEFINE(inst, __VA_ARGS__);                                                      \
    BEHAVIOR_DEFINE(DT_DRV_INST(inst))

/**
 * Syscall wrapper for zmk_behavior_get_binding().
 *
 * Use zmk_behavior_get_binding() in application code instead.
 */
__syscall const struct device *behavior_get_binding(const char *name);

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
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_driver_api *api = (const struct behavior_driver_api *)dev->api;

    if (api->binding_convert_central_state_dependent_params == NULL) {
        return 0;
    }

    return api->binding_convert_central_state_dependent_params(binding, event);
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

/**
 * @brief Determine where the behavior should be run
 * @param behavior Pointer to the device structure for the driver instance.
 *
 * @retval Zero if successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_get_parameter_domains(const struct device *behavior,
                                             struct behavior_parameter_metadata *param_metadata);

static inline int
z_impl_behavior_get_parameter_domains(const struct device *behavior,
                                      struct behavior_parameter_metadata *param_metadata) {
    if (behavior == NULL || param_metadata == NULL) {
        return -EINVAL;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)behavior->api;

    if (api->get_parameter_domains) {
        return api->get_parameter_domains(behavior, param_metadata);
    } else if (api->custom_parameters) {
        param_metadata->type = BEHAVIOR_PARAMETER_METADATA_CUSTOM;
        param_metadata->custom = api->custom_parameters;
    } else {
        param_metadata->type = BEHAVIOR_PARAMETER_METADATA_STANDARD;
        param_metadata->standard.param1 = api->param1_standard_domain;
        param_metadata->standard.param2 = api->param2_standard_domain;
    }

    return 0;
}

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

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
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);

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
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);

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
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);

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
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);

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
