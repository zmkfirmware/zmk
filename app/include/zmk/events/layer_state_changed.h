/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_layer_state_changed {
    uint8_t layer;
    bool state;
    bool locked;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_layer_state_changed);

static inline int raise_layer_state_changed(uint8_t layer, bool state, bool locked) {
    return raise_zmk_layer_state_changed((struct zmk_layer_state_changed){
        .layer = layer, .state = state, .locked = locked, .timestamp = k_uptime_get()});
}
