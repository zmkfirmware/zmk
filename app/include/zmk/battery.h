/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

uint8_t zmk_battery_state_of_charge(void);
bool zmk_battery_is_charging(void);
bool zmk_is_externally_powered(void);
