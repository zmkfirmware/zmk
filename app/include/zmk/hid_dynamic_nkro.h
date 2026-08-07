/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

enum zmk_hid_dynamic_nkro_mode {
    ZMK_HID_DYNAMIC_NKRO_MODE_NKRO = 0,
    ZMK_HID_DYNAMIC_NKRO_MODE_HKRO = 1,
};

/**
 * The HID report mode active for the current boot session. Decided once, early in boot,
 * before USB/BLE HID are initialized, since the report descriptor/size they register cannot
 * change without a full re-enumeration (i.e. a reboot).
 */
enum zmk_hid_dynamic_nkro_mode zmk_hid_dynamic_nkro_get_mode(void);

/**
 * Persist a new mode to flash. Does not take effect until the next reboot; callers that want
 * the change to apply are expected to trigger one (see behavior_dynamic_nkro.c).
 */
int zmk_hid_dynamic_nkro_set_mode(enum zmk_hid_dynamic_nkro_mode mode);
