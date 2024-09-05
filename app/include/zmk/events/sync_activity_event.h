/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/hid_indicators_types.h>
#include <zmk/event_manager.h>

struct zmk_sync_activity_event {
    int32_t central_inactive_duration;
};

ZMK_EVENT_DECLARE(zmk_sync_activity_event);
