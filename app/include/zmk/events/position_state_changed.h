/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>
#include <bluetooth/addr.h>

#if IS_ENABLED(CONFIG_ZMK_BLE)
typedef const bt_addr_le_t *zmk_position_state_changed_source_t;
#else
typedef void *zmk_position_state_changed_source_t;
#endif

struct zmk_position_state_changed {
    zmk_position_state_changed_source_t source;
    uint32_t position;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_position_state_changed);
