/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_split_wpm_state_changed {
    uint8_t wpm;
};

ZMK_EVENT_DECLARE(zmk_split_wpm_state_changed);


// #pragma once

// #include <zephyr/kernel.h>
// #include <zmk/event_manager.h>

// struct zmk_split_wpm_state_changed {
//     struct zmk_event_header header;
//     uint8_t wpm;
// };

// ZMK_EVENT_DECLARE(zmk_split_wpm_state_changed);

// #pragma once

// #include <zephyr/kernel.h>
// #include <zmk/events_manager.h>  // ✅ Quan trọng! Cần include file này để có struct zmk_event_header

// struct zmk_split_wpm_state_changed {
//     struct zmk_event_header header;
//     uint8_t wpm;
// };

// ZMK_EVENT_DECLARE(zmk_split_wpm_state_changed);
