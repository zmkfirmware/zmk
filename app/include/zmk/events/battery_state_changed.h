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
};

ZMK_EVENT_DECLARE(zmk_battery_state_changed);

struct zmk_peripheral_battery_state_changed {
    uint8_t source;
    // TODO: Other battery channels
    uint8_t state_of_charge;
};

ZMK_EVENT_DECLARE(zmk_peripheral_battery_state_changed);

// Raised on split peripherals, see CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY.
struct zmk_central_battery_state_changed {
    uint8_t state_of_charge;
    bool usb_powered;
};

ZMK_EVENT_DECLARE(zmk_central_battery_state_changed);
