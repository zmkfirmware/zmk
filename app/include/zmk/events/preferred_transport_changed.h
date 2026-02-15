/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

#include <zmk/endpoints_types.h>
#include <zmk/event_manager.h>

struct zmk_preferred_transport_changed {
    enum zmk_transport transport;
};

ZMK_EVENT_DECLARE(zmk_preferred_transport_changed);