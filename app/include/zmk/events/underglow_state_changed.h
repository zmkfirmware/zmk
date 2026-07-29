/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/rgb_underglow.h>

struct zmk_underglow_state_changed {
    struct zmk_underglow_state state;
};

ZMK_EVENT_DECLARE(zmk_underglow_state_changed);