/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zmk/events/backlight_state_changed.h>

ZMK_EVENT_IMPL(zmk_backlight_state_changed);