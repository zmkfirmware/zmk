/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/split/transport/types.h>

struct zmk_split_transport_changed {
    uint32_t addr;
    struct zmk_split_transport_status status;
};

ZMK_EVENT_DECLARE(zmk_split_transport_changed)