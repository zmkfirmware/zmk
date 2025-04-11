/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>

#include <zmk/split/transport/types.h>

typedef int (*zmk_split_central_report_event_callback_t)(
    const struct zmk_split_transport_peripheral_event *event);

struct zmk_split_transport_peripheral_api {
    zmk_split_central_report_event_callback_t report_event;
};

struct zmk_split_transport_peripheral {
    const struct zmk_split_transport_peripheral_api *api;
};

int zmk_split_transport_peripheral_command_handler(
    const struct zmk_split_transport_peripheral *transport,
    struct zmk_split_transport_central_command cmd);

#define ZMK_SPLIT_TRANSPORT_PERIPHERAL_REGISTER(name, _api)                                        \
    STRUCT_SECTION_ITERABLE(zmk_split_transport_peripheral, name) = {                              \
        .api = _api,                                                                               \
    };
