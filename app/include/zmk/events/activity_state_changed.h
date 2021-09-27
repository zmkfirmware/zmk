/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>
#include <zmk/activity.h>

struct zmk_activity_state_changed {
    enum zmk_activity_state state;
};

ZMK_EVENT_DECLARE(zmk_activity_state_changed);