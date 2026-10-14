/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zephyr/device.h>

#include <zmk/ble.h>

struct zmk_ble_advertising_status_changed {
    enum zmk_ble_advertising_type advertising_status;
};

ZMK_EVENT_DECLARE(zmk_ble_advertising_status_changed);
