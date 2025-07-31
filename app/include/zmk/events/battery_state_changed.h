/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_battery_state_changed {
    // TODO: Other battery channels
    uint8_t state_of_charge;
    bool charging;
};

ZMK_EVENT_DECLARE(zmk_battery_state_changed);

struct zmk_peripheral_battery_state_changed {
    uint8_t source;
    // TODO: Other battery channels
    // Charging state not broadcast over BAS so no need to have it in peripheral event
    uint8_t state_of_charge;
};

ZMK_EVENT_DECLARE(zmk_peripheral_battery_state_changed);