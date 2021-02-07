/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>
#include <zmk/matrix.h>

struct zmk_position_state_changed {
    uint32_t position;
    bool state;
    int64_t timestamp;
    int32_t trace_id;
};

ZMK_EVENT_DECLARE(zmk_position_state_changed);

uint32_t zmk_get_event_trace_id(uint32_t position, bool pressed);