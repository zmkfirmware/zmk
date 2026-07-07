/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>

#include <zmk/split/transport/types.h>

struct zmk_split_transport_central;

typedef int (*zmk_split_transport_central_status_changed_cb_t)(
    const struct zmk_split_transport_central *transport, struct zmk_split_transport_status status);

typedef int (*zmk_split_transport_central_send_command_t)(
    uint8_t source, struct zmk_split_transport_central_command cmd);
typedef int (*zmk_split_transport_central_get_available_source_ids_t)(uint8_t *sources);
typedef int (*zmk_split_transport_central_set_status_callback_t)(
    zmk_split_transport_central_status_changed_cb_t cb);

struct zmk_split_transport_central_api {
    zmk_split_transport_central_send_command_t send_command;
    zmk_split_transport_central_get_available_source_ids_t get_available_source_ids;
    zmk_split_transport_set_enabled_t set_enabled;
    zmk_split_transport_get_status_t get_status;
    zmk_split_transport_central_set_status_callback_t set_status_callback;
};

struct zmk_split_transport_central {
    const struct zmk_split_transport_central_api *api;
};

int zmk_split_transport_central_peripheral_event_handler(
    const struct zmk_split_transport_central *transport, uint8_t source,
    struct zmk_split_transport_peripheral_event ev);

#define ZMK_SPLIT_TRANSPORT_CENTRAL_REGISTER(name, _api, priority)                                 \
    STRUCT_SECTION_ITERABLE_NAMED(zmk_split_transport_central, _CONCAT(priority, _##name),         \
                                  name) = {                                                        \
        .api = _api,                                                                               \
    };
