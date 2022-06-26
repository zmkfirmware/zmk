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
#define ZMK_BLE_PROFILE_COUNT (CONFIG_BT_MAX_PAIRED - 1)
#define ZMK_BLE_SPLIT_PERIPHERAL_COUNT 1
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
void zmk_ble_set_peripheral_addr(bt_addr_le_t *addr);
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */
