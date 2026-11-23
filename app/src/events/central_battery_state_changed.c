/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zmk/events/battery_state_changed.h>

ZMK_EVENT_IMPL(zmk_central_battery_state_changed);
