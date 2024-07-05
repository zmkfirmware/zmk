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

struct behavior_parameter_value_metadata {
    char *display_name;

    union {
        uint32_t value;
        struct {
            int32_t min;
            int32_t max;
        } range;
    };

    enum {
        BEHAVIOR_PARAMETER_VALUE_TYPE_NIL = 0,
        BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE = 1,
        BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE = 2,
        BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE = 3,
        BEHAVIOR_PARAMETER_VALUE_TYPE_LAYER_ID = 4,
    } type;
};

struct behavior_parameter_metadata_set {
    size_t param1_values_len;
    const struct behavior_parameter_value_metadata *param1_values;

    size_t param2_values_len;
    const struct behavior_parameter_value_metadata *param2_values;
};

struct behavior_parameter_metadata {
    size_t sets_len;
    const struct behavior_parameter_metadata_set *sets;
};

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
typedef int (*behavior_get_parameter_metadata_t)(
    const struct device *behavior, struct behavior_parameter_metadata *param_metadata);
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
    behavior_get_parameter_metadata_t get_parameter_metadata;
    const struct behavior_parameter_metadata *parameter_metadata;
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};
/**
 * @endcond
 */

struct zmk_behavior_metadata {
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    const char *display_name;
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

struct zmk_behavior_ref {
    const struct device *device;
    const struct zmk_behavior_metadata metadata;
};

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS)

struct zmk_behavior_local_id_map {
    const struct device *device;
    zmk_behavior_local_id_t local_id;
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS)

#define ZMK_BEHAVIOR_REF_DT_NAME(node_id) _CONCAT(zmk_behavior_, DEVICE_DT_NAME_GET(node_id))

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

#define ZMK_BEHAVIOR_METADATA_INITIALIZER(node_id)                                                 \
    { .display_name = DT_PROP_OR(node_id, display_name, DEVICE_DT_NAME(node_id)), }

#else

#define ZMK_BEHAVIOR_METADATA_INITIALIZER(node_id)                                                 \
    {}

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

#define ZMK_BEHAVIOR_REF_INITIALIZER(node_id, _dev)                                                \
    { .device = _dev, .metadata = ZMK_BEHAVIOR_METADATA_INITIALIZER(node_id), }

#define ZMK_BEHAVIOR_LOCAL_ID_MAP_INITIALIZER(node_id, _dev)                                       \
    { .device = _dev, }

#define ZMK_BEHAVIOR_REF_DEFINE(name, node_id, _dev)                                               \
    static const STRUCT_SECTION_ITERABLE(zmk_behavior_ref, name) =                                 \
        ZMK_BEHAVIOR_REF_INITIALIZER(node_id, _dev);                                               \
    COND_CODE_1(IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS),                                         \
                (static const STRUCT_SECTION_ITERABLE(zmk_behavior_local_id_map,                   \
                                                      _CONCAT(_zmk_behavior_local_id_map, name)) = \
                     ZMK_BEHAVIOR_LOCAL_ID_MAP_INITIALIZER(node_id, _dev)),                        \
                ());

#define ZMK_BEHAVIOR_REF_DT_DEFINE(node_id)                                                        \
    ZMK_BEHAVIOR_REF_DEFINE(ZMK_BEHAVIOR_REF_DT_NAME(node_id), node_id, DEVICE_DT_GET(node_id))

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
 * @brief Validate a given behavior binding is valid, including parameter validation
 * if the metadata feature is enablued.
 *
 * @param binding The behavior binding to validate.
 *
 * @retval 0 if the passed in binding is valid.
 * @retval -ENODEV if the binding references a non-existant behavior.
 * @retval -EINVAL if parameters are not valid for the behavior metadata.
 */
int zmk_behavior_validate_binding(const struct zmk_behavior_binding *binding);

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

int zmk_behavior_get_empty_param_metadata(const struct device *dev,
                                          struct behavior_parameter_metadata *metadata);

/**
 * @brief Validate a given behavior parameters match the behavior metadata.
 *
 * @param metadata The behavior metadata to validate against
 * @param param1 The first parameter value
 * @param param2 The second parameter value
 *
 * @retval 0 if the passed in parameters are valid.
 * @retval -ENODEV if metadata is NULL.
 * @retval -EINVAL if parameters are not valid for the metadata.
 */
int zmk_behavior_check_params_match_metadata(const struct behavior_parameter_metadata *metadata,
                                             uint32_t param1, uint32_t param2);
/**
 * @brief Validate a given behavior parameter matches the behavior metadata parameter values.
 *
 * @param values The values to validate against
 * @param values_len How many values to check
 * @param param The value to check.
 *
 * @retval 0 if the passed in parameter is valid.
 * @retval -ENODEV if values is NULL.
 * @retval -EINVAL if parameter is not valid for the value metadata.
 */
int zmk_behavior_validate_param_values(const struct behavior_parameter_value_metadata *values,
                                       size_t values_len, uint32_t param);

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

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
__syscall int behavior_get_parameter_metadata(const struct device *behavior,
                                              struct behavior_parameter_metadata *param_metadata);

static inline int
z_impl_behavior_get_parameter_metadata(const struct device *behavior,
                                       struct behavior_parameter_metadata *param_metadata) {
    if (behavior == NULL || param_metadata == NULL) {
        return -EINVAL;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)behavior->api;

    if (api->get_parameter_metadata) {
        return api->get_parameter_metadata(behavior, param_metadata);
    } else if (api->parameter_metadata) {
        *param_metadata = *api->parameter_metadata;
    } else {
        return -ENODEV;
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
