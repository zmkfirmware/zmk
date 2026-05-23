/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

struct peripheral_ble_state {
    bool connected;
};

struct peripheral_ble_state zmk_get_ble_peripheral_state();
int zmk_rgb_underglow_set_color_ble_peripheral(struct peripheral_ble_state ps);
