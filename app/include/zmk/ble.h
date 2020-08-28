/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <bluetooth/addr.h>
#include <zmk/keys.h>

int zmk_ble_adv_pause();
int zmk_ble_adv_resume();

int zmk_ble_identity_clear();
int zmk_ble_identity_next();
int zmk_ble_identity_prev();
int zmk_ble_identity_select(u8_t index);

int zmk_ble_unpair_all();
bool zmk_ble_handle_key_user(struct zmk_key_event *key_event);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)
void zmk_ble_set_peripheral_addr(bt_addr_le_t *addr);
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL) */