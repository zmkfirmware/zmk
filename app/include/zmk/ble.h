/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/keys.h>
#include <zmk/ble/profile.h>

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
bool zmk_ble_handle_key_user(struct zmk_key_event *key_event);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)
void zmk_ble_set_peripheral_addr(bt_addr_le_t *addr);
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL) */