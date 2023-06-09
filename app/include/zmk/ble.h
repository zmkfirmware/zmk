/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/keys.h>
#include <zmk/ble/profile.h>

#define ZMK_BLE_IS_CENTRAL                                                                         \
    (IS_ENABLED(CONFIG_ZMK_SPLIT) && IS_ENABLED(CONFIG_ZMK_BLE) &&                                 \
     IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

#if ZMK_BLE_IS_CENTRAL
#define ZMK_BLE_PROFILE_COUNT (CONFIG_BT_MAX_PAIRED - CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS)
#define ZMK_SPLIT_BLE_PERIPHERAL_COUNT CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS
#else
#define ZMK_BLE_PROFILE_COUNT CONFIG_BT_MAX_PAIRED
#endif

int zmk_ble_clear_bonds();
int zmk_ble_prof_next();
int zmk_ble_prof_prev();
int zmk_ble_prof_select(uint8_t index);

int zmk_ble_active_profile_index();
bt_addr_le_t *zmk_ble_active_profile_addr();
bool zmk_ble_active_profile_is_open();
bool zmk_ble_active_profile_is_connected();
char *zmk_ble_active_profile_name();

int zmk_ble_unpair_all();

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
int zmk_ble_put_peripheral_addr(const bt_addr_le_t *addr);
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */
