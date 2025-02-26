/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>

#include <zmk/split/transport/types.h>

typedef int (*zmk_split_transport_central_send_command_t)(
    uint8_t source, struct zmk_split_transport_central_command cmd);
typedef int (*zmk_split_transport_central_get_available_source_ids_t)(uint8_t *sources);

struct zmk_split_transport_central_api {
    zmk_split_transport_central_send_command_t send_command;
    zmk_split_transport_central_get_available_source_ids_t get_available_source_ids;
};

struct zmk_split_transport_central {
    const struct zmk_split_transport_central_api *api;
};

int zmk_split_transport_central_peripheral_event_handler(
    const struct zmk_split_transport_central *transport, uint8_t source,
    struct zmk_split_transport_peripheral_event ev);

#define ZMK_SPLIT_TRANSPORT_CENTRAL_REGISTER(name, _api)                                           \
    STRUCT_SECTION_ITERABLE(zmk_split_transport_central, name) = {                                 \
        .api = _api,                                                                               \
    };
