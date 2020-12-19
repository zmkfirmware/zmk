/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>
#include <zmk/behavior.h>

struct zmk_behavior_state_changed {
    struct zmk_behavior_binding binding;
    bool pressed;
    int layer;
    uint32_t position;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_behavior_state_changed);
