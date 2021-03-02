/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>

struct zmk_layer_state_changed {
    zmk_keymap_layers_state_t prior_state;
    zmk_keymap_layers_state_t state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_layer_state_changed);

static inline struct zmk_layer_state_changed_event *
create_layer_state_changed(zmk_keymap_layers_state_t prior_state, zmk_keymap_layers_state_t state) {

    return new_zmk_layer_state_changed((struct zmk_layer_state_changed){
        .prior_state = prior_state, .state = state, .timestamp = k_uptime_get()});
}
