/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stddef.h>
#include <sys/errno.h>
#include <zephyr/device.h>
#include <zephyr/toolchain/common.h>
#include <zmk/behavior.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * Character map driver API definition and system call entry points.
 *
 * (Internal use only.)
 */

typedef int (*character_map_codepoint_to_binding_t)(const struct device *device, uint32_t codepoint,
                                                    struct zmk_behavior_binding *binding);

__subsystem struct character_map_driver_api {
    character_map_codepoint_to_binding_t codepoint_to_binding;
};
/**
 * @endcond
 */

/**
 * @brief Map a Unicode codepoint to a behavior binding.
 * @param charmap Pointer to the device structure for the driver instance.
 * @param codepoint Unicode codepoint to map.
 * @param binding Corresponding behavior binding is written here if successful.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int character_map_codepoint_to_binding(const struct device *charmap, uint32_t codepoint,
                                                 struct zmk_behavior_binding *binding);

static inline int z_impl_character_map_codepoint_to_binding(const struct device *charmap,
                                                            uint32_t codepoint,
                                                            struct zmk_behavior_binding *binding) {
    const struct character_map_driver_api *api =
        (const struct character_map_driver_api *)charmap->api;

    if (api->codepoint_to_binding == NULL) {
        return -ENOTSUP;
    }

    return api->codepoint_to_binding(charmap, codepoint, binding);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/character_map.h>
