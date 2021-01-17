/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>

struct zmk_position_state_changed_data {
    uint32_t position;
    bool state;
    int64_t timestamp;
};

struct position_state_changed {
    struct zmk_event_header header;
    struct zmk_position_state_changed_data data;
};

ZMK_EVENT_DECLARE(position_state_changed);