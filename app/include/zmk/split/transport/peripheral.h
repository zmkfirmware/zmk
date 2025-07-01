/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>

#include <zmk/split/transport/types.h>

struct zmk_split_transport_peripheral;

typedef int (*zmk_split_transport_peripheral_status_changed_cb_t)(
    const struct zmk_split_transport_peripheral *transport,
    struct zmk_split_transport_status status);

typedef int (*zmk_split_transport_peripheral_report_event_callback_t)(
    const struct zmk_split_transport_peripheral_event *event);
typedef int (*zmk_split_transport_peripheral_set_status_callback_t)(
    zmk_split_transport_peripheral_status_changed_cb_t cb);

struct zmk_split_transport_peripheral_api {
    zmk_split_transport_peripheral_report_event_callback_t report_event;
    zmk_split_transport_set_enabled_t set_enabled;
    zmk_split_transport_get_status_t get_status;
    zmk_split_transport_peripheral_set_status_callback_t set_status_callback;
};

struct zmk_split_transport_peripheral {
    const struct zmk_split_transport_peripheral_api *api;
};

int zmk_split_transport_peripheral_command_handler(
    const struct zmk_split_transport_peripheral *transport,
    struct zmk_split_transport_central_command cmd);

#define ZMK_SPLIT_TRANSPORT_PERIPHERAL_REGISTER(name, _api, priority)                              \
    STRUCT_SECTION_ITERABLE_NAMED(zmk_split_transport_peripheral, _CONCAT(priority, _##name),      \
                                  name) = {                                                        \
        .api = _api,                                                                               \
    };
