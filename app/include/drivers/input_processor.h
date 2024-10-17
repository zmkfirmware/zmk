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
#include <zephyr/input/input.h>

struct zmk_input_processor_state {
    float *remainder;
};

// TODO: Need the ability to store remainders? Some data passed in?
typedef int (*zmk_input_processor_handle_event_callback_t)(const struct device *dev,
                                                           struct input_event *event,
                                                           uint32_t param1, uint32_t param2,
                                                           struct zmk_input_processor_state *state);

__subsystem struct zmk_input_processor_driver_api {
    zmk_input_processor_handle_event_callback_t handle_event;
};

__syscall int zmk_input_processor_handle_event(const struct device *dev, struct input_event *event,
                                               uint32_t param1, uint32_t param2,
                                               struct zmk_input_processor_state *state);

static inline int z_impl_zmk_input_processor_handle_event(const struct device *dev,
                                                          struct input_event *event,
                                                          uint32_t param1, uint32_t param2,
                                                          struct zmk_input_processor_state *state) {
    const struct zmk_input_processor_driver_api *api =
        (const struct zmk_input_processor_driver_api *)dev->api;

    if (api->handle_event == NULL) {
        return -ENOTSUP;
    }

    return api->handle_event(dev, event, param1, param2, state);
}

#include <syscalls/input_processor.h>
