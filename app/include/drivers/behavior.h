/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <zmk/keys.h>

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

typedef int (*behavior_position_callback_t)(struct device *dev, u32_t position);
typedef int (*behavior_keymap_binding_callback_t)(struct device *dev, u32_t position, u32_t param1, u32_t param2);
typedef int (*behavior_keycode_callback_t)(struct device *dev, u32_t keycode);
typedef int (*behavior_modifiers_callback_t)(struct device *dev, zmk_mod_flags modifiers);

__subsystem struct behavior_driver_api {
	behavior_position_callback_t position_pressed;
	behavior_position_callback_t position_released;
	behavior_keymap_binding_callback_t binding_pressed;
	behavior_keymap_binding_callback_t binding_released;
	behavior_keycode_callback_t keycode_pressed;
	behavior_keycode_callback_t keycode_released;
	behavior_modifiers_callback_t modifiers_pressed;
	behavior_modifiers_callback_t modifiers_released;
};
/**
 * @endcond
 */

/**
 * @brief Handle the key position being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param position They key position that was pressed
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_position_pressed(struct device *dev, u32_t position);

static inline int z_impl_behavior_position_pressed(struct device *dev, u32_t position)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->position_pressed == NULL) {
		return -ENOTSUP;
	}

	return api->position_pressed(dev, position);
}

/**
 * @brief Handle the key position being released
 * @param dev Pointer to the device structure for the driver instance.
 * @param position They key position that was released
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_position_released(struct device *dev, u32_t position);

static inline int z_impl_behavior_position_released(struct device *dev, u32_t position)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->position_released == NULL) {
		return -ENOTSUP;
	}

	return api->position_released(dev, position);
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
__syscall int behavior_keymap_binding_pressed(struct device *dev, u32_t position, u32_t param1, u32_t param2);

static inline int z_impl_behavior_keymap_binding_pressed(struct device *dev, u32_t position, u32_t param1, u32_t param2)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->binding_pressed == NULL) {
		return -ENOTSUP;
	}

	return api->binding_pressed(dev, position, param1, param2);
}

/**
 * @brief Handle the assigned position being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param param1 User parameter specified at time of behavior assignment.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_keymap_binding_released(struct device *dev, u32_t position, u32_t param1, u32_t param2);

static inline int z_impl_behavior_keymap_binding_released(struct device *dev, u32_t position, u32_t param1, u32_t param2)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->binding_released == NULL) {
		return -ENOTSUP;
	}

	return api->binding_released(dev, position, param1, param2);
}


/**
 * @brief Handle the keycode being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param keycode The keycode that is being pressed.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_keycode_pressed(struct device *dev, u32_t keycode);

static inline int z_impl_behavior_keycode_pressed(struct device *dev, u32_t keycode)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->keycode_pressed == NULL) {
		return -ENOTSUP;
	}

	return api->keycode_pressed(dev, keycode);
}


/**
 * @brief Handle the keycode being released
 * @param dev Pointer to the device structure for the driver instance.
 * @param keycode The keycode that is being pressed.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_keycode_released(struct device *dev, u32_t keycode);

static inline int z_impl_behavior_keycode_released(struct device *dev, u32_t keycode)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->keycode_released == NULL) {
		return -ENOTSUP;
	}

	return api->keycode_released(dev, keycode);
}


/**
 * @brief Handle the keycode being pressed
 * @param dev Pointer to the device structure for the driver instance.
 * @param keycode The keycode that is being pressed.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_modifiers_pressed(struct device *dev, zmk_mod_flags modifiers);

static inline int z_impl_behavior_modifiers_pressed(struct device *dev, zmk_mod_flags modifiers)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->modifiers_pressed == NULL) {
		return -ENOTSUP;
	}

	return api->modifiers_pressed(dev, modifiers);
}


/**
 * @brief Handle the keycode being released
 * @param dev Pointer to the device structure for the driver instance.
 * @param keycode The keycode that is being pressed.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int behavior_modifiers_released(struct device *dev, zmk_mod_flags modifiers);

static inline int z_impl_behavior_modifiers_released(struct device *dev, zmk_mod_flags modifiers)
{
	const struct behavior_driver_api *api =
			(const struct behavior_driver_api *)dev->driver_api;

	if (api->modifiers_released == NULL) {
		return -ENOTSUP;
	}

	return api->modifiers_released(dev, modifiers);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/behavior.h>
