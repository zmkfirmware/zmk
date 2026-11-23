/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zephyr/device.h>

#include <zmk/ble/profile.h>

struct zmk_ble_active_profile_changed {
    uint8_t index;
    // NULL on split peripherals, see CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE.
    struct zmk_ble_profile *profile;
};

ZMK_EVENT_DECLARE(zmk_ble_active_profile_changed);
