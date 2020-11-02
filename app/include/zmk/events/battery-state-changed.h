/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event-manager.h>

struct battery_state_changed {
    struct zmk_event_header header;
    // TODO: Other battery channels
    u8_t state_of_charge;
};

ZMK_EVENT_DECLARE(battery_state_changed);