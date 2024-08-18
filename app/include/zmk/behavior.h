/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>

#define ZMK_BEHAVIOR_OPAQUE 0
#define ZMK_BEHAVIOR_TRANSPARENT 1

typedef uint16_t zmk_behavior_local_id_t;

struct zmk_behavior_binding {
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS_IN_BINDINGS)
    zmk_behavior_local_id_t local_id;
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS_IN_BINDINGS)
    const char *behavior_dev;
    uint32_t param1;
    uint32_t param2;
};

struct zmk_behavior_binding_event {
    int layer;
    uint32_t position;
    int64_t timestamp;
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    uint8_t source;
#endif
};

/**
 * @brief Get a const struct device* for a behavior from its @p name field.
 *
 * @param name Behavior name to search for.
 *
 * @retval Pointer to the device structure for the behavior with the given name.
 * @retval NULL if the behavior is not found or its initialization function failed.
 *
 * @note This is equivalent to device_get_binding(), except it only searches
 * behavior devices, so it is faster and there is no chance of it returning an
 * unrelated node which shares the same name as a behavior.
 */
const struct device *zmk_behavior_get_binding(const char *name);

/**
 * @brief Invoke a behavior given its binding and invoking event details.
 *
 * @param src_binding Behavior binding to invoke.
 * @param event The binding event struct containing details of the event that invoked it.
 * @param pressed Whether the binding is pressed or released.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
int zmk_behavior_invoke_binding(const struct zmk_behavior_binding *src_binding,
                                struct zmk_behavior_binding_event event, bool pressed);

/**
 * @brief Get a local ID for a behavior from its @p name field.
 *
 * @param name Behavior name to search for.
 *
 * @retval The local ID value that can be used to reference the behavior later, across reboots.
 * @retval UINT16_MAX if the behavior is not found or its initialization function failed.
 */
zmk_behavior_local_id_t zmk_behavior_get_local_id(const char *name);

/**
 * @brief Get a behavior name for a behavior from its @p local_id .
 *
 * @param local_id Behavior local ID used to search for the behavior
 *
 * @retval The name of the behavior that is associated with that local ID.
 * @retval NULL if the behavior is not found or its initialization function failed.
 */
const char *zmk_behavior_find_behavior_name_from_local_id(zmk_behavior_local_id_t local_id);
