/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

#include <zephyr/sys/util.h>

// Values match the BLE Battery Level Status charge state field.
enum zmk_battery_charge_state {
    ZMK_BATTERY_CHARGE_STATE_UNKNOWN = 0,
    ZMK_BATTERY_CHARGE_STATE_CHARGING = 1,
    ZMK_BATTERY_CHARGE_STATE_DISCHARGING = 2,
    ZMK_BATTERY_CHARGE_STATE_FULL = 3,
};

uint8_t zmk_battery_state_of_charge(void);

// Sample now instead of waiting for the next report interval.
void zmk_battery_update_now(void);

#if IS_ENABLED(CONFIG_ZMK_BATTERY_CHARGE_STATUS)

enum zmk_battery_charge_state zmk_battery_charge_state(void);

#else

// No charger status pin, so there is nothing to report from. USB power is not a
// substitute: it stays asserted once the battery is full.
static inline enum zmk_battery_charge_state zmk_battery_charge_state(void) {
    return ZMK_BATTERY_CHARGE_STATE_UNKNOWN;
}

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_CHARGE_STATUS)
