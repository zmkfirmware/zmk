/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

uint8_t zmk_battery_state_of_charge();
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
uint8_t zmk_battery_state_of_peripheral_charge();
#endif
