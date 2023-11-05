/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

enum zmk_ble_auth_mode {
    // Not authenticating
    ZMK_BLE_AUTH_MODE_NONE,
    // User must confirm keyboard and host are displaying the same passkey.
    ZMK_BLE_AUTH_MODE_PASSKEY_CONFIRM,
    // Keyboard is dispaying a passkey, and user must enter it on the host.
    ZMK_BLE_AUTH_MODE_PASSKEY_DISPLAY,
    // Host is displaying a passkey, and user must enter it on the keyboard.
    ZMK_BLE_AUTH_MODE_PASSKEY_ENTRY,
};

struct zmk_ble_auth_state {
    enum zmk_ble_auth_mode mode;
    // Index of the profile being authenticated.
    uint8_t profile_index;
    // In passkey entry mode, the index of the next digit to enter.
    uint8_t cursor_index;
    // The current passkey. The value will be in the range 0 - 999999 and should
    // be padded with zeros so that six digits are always shown. E.g. the value
    // 37 should be shown as 000037.
    unsigned int passkey;
};
