/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>

struct zmk_layer_state_changed {
    uint8_t layer;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_layer_state_changed);

static inline struct zmk_layer_state_changed_event *create_layer_state_changed(uint8_t layer,
                                                                               bool state) {
    return new_zmk_layer_state_changed((struct zmk_layer_state_changed){
        .layer = layer, .state = state, .timestamp = k_uptime_get()});
}
