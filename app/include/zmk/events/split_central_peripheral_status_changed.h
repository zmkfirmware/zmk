/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/central.h>

struct zmk_split_central_peripheral_status_changed {
    enum peripheral_slot_state state;
    uint8_t slot;
};

ZMK_EVENT_DECLARE(zmk_split_central_peripheral_status_changed);
