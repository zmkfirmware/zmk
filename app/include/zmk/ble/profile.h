/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <bluetooth/addr.h>

#define ZMK_BLE_PROFILE_NAME_MAX 15

struct zmk_ble_profile {
    char name[ZMK_BLE_PROFILE_NAME_MAX];
    bt_addr_le_t peer;
};
