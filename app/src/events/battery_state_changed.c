/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zmk/events/battery_state_changed.h>

ZMK_EVENT_IMPL(zmk_battery_state_changed);

ZMK_EVENT_IMPL(zmk_peripheral_battery_state_changed);