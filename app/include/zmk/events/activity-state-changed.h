/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event-manager.h>
#include <zmk/activity.h>

struct activity_state_changed {
    struct zmk_event_header header;
    enum zmk_activity_state state;
};

ZMK_EVENT_DECLARE(activity_state_changed);

static inline struct activity_state_changed *
create_activity_state_changed(enum zmk_activity_state state) {
    struct activity_state_changed *ev = new_activity_state_changed();
    ev->state = state;

    return ev;
}