/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * Behavior driver API definition and system call entry points.
 *
 * (Internal use only.)
 */

typedef int (*behavior_position_pressed_t)(struct device *dev, u32_t param1, u32_t param2);
typedef int (*behavior_position_released_t)(struct device *dev, u32_t param1, u32_t param2);

__subsystem struct behavior_driver_api {
	behavior_position_pressed_t position_pressed;
	behavior_position_released_t position_released;
};
/**
 * @endcond
 */

/**
 * @brief Handle the assigned position being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param param1 User parameter specified at time of behavior assignment.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_position_pressed(struct device *dev, u32_t param1, u32_t param2);

static inline int z_impl_behavior_position_pressed(struct device *dev, u32_t param1, u32_t param2)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->position_pressed == NULL) {
		return -ENOTSUP;
	}

	return api->position_pressed(dev, param1, param2);
}

/**
 * @brief Handle the assigned position being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param param1 User parameter specified at time of behavior assignment.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_position_released(struct device *dev, u32_t param1, u32_t param2);

static inline int z_impl_behavior_position_released(struct device *dev, u32_t param1, u32_t param2)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->position_released == NULL) {
		return -ENOTSUP;
	}

	return api->position_released(dev, param1, param2);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/behavior.h>
