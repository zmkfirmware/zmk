/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/backlight.h>

struct zmk_backlight_state_changed {
    struct zmk_backlight_state state;
};

ZMK_EVENT_DECLARE(zmk_backlight_state_changed);