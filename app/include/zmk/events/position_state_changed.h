/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>
struct zmk_position_state_changed {
    uint32_t position;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_position_state_changed);