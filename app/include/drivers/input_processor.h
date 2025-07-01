/*
 * Copyright (c) 2024 The ZMK Contributors
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

#define ZMK_INPUT_PROC_CONTINUE 0
#define ZMK_INPUT_PROC_STOP 1

struct zmk_input_processor_entry {
    const struct device *dev;
    uint32_t param1;
    uint32_t param2;
    bool track_remainders;
};

#define ZMK_INPUT_PROCESSOR_ENTRY_AT_IDX(idx, n)                                                   \
    {                                                                                              \
        .dev = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(n, input_processors, idx)),                         \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(n, input_processors, idx, param1), (0),       \
                              (DT_PHA_BY_IDX(n, input_processors, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(n, input_processors, idx, param2), (0),       \
                              (DT_PHA_BY_IDX(n, input_processors, idx, param2))),                  \
        .track_remainders =                                                                        \
            COND_CODE_1(DT_PROP(DT_PHANDLE_BY_IDX(n, input_processors, idx), track_remainders),    \
                        (true), (false)),                                                          \
    }

struct zmk_input_processor_state {
    uint8_t input_device_index;
    int16_t *remainder;
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
