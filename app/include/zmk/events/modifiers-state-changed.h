/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/keys.h>
#include <zmk/event-manager.h>

struct modifiers_state_changed {
    struct zmk_event_header header;
    zmk_mod_flags modifiers;
    bool state;
};

ZMK_EVENT_DECLARE(modifiers_state_changed);

inline struct modifiers_state_changed *create_modifiers_state_changed(zmk_mod_flags modifiers,
                                                                      bool state) {
    struct modifiers_state_changed *ev = new_modifiers_state_changed();
    ev->modifiers = modifiers;
    ev->state = state;

    return ev;
}