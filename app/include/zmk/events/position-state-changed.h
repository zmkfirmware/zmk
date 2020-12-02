/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event-manager.h>

struct position_state_changed {
    struct zmk_event_header header;
    uint32_t position;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(position_state_changed);