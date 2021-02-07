/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <zmk/events/position_state_changed.h>

ZMK_EVENT_IMPL(zmk_position_state_changed);

static uint32_t zmk_last_event_trace_id = 0;
static uint32_t zmk_event_trace_ids[ZMK_KEYMAP_LEN] = {0};

uint32_t zmk_get_event_trace_id(uint32_t position, bool pressed) {
    if (pressed) {
        zmk_last_event_trace_id++;
        zmk_event_trace_ids[position] = zmk_last_event_trace_id;
    }
    return zmk_event_trace_ids[position];
}
