/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/event_manager.h>
#include <zmk/ble/auth.h>

struct zmk_ble_auth_state_changed {
    struct zmk_ble_auth_state state;
};

ZMK_EVENT_DECLARE(zmk_ble_auth_state_changed);
