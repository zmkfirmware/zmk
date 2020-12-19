/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>
#include <zmk/behavior.h>

struct behavior_state_changed {
    struct zmk_event_header header;
    char *behavior_dev;
    uint32_t param1;
    uint32_t param2;
    bool pressed;
    int layer;
    uint32_t position;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(behavior_state_changed);

static inline struct behavior_state_changed *
create_behavior_state_changed_from_binding(struct zmk_behavior_binding *binding, bool pressed,
                                           int layer, uint32_t position, int64_t timestamp) {
    struct behavior_state_changed *ev = new_behavior_state_changed();
    ev->behavior_dev = binding->behavior_dev;
    ev->param1 = binding->param1;
    ev->param2 = binding->param2;
    ev->pressed = pressed;
    ev->layer = layer;
    ev->position = position;
    ev->timestamp = timestamp;
    return ev;
}

static inline struct behavior_state_changed *
create_behavior_state_changed(char *behavior_dev, uint32_t param1, uint32_t param2, bool pressed,
                              int layer, uint32_t position, int64_t timestamp) {
    struct behavior_state_changed *ev = new_behavior_state_changed();
    ev->behavior_dev = behavior_dev;
    ev->param1 = param1;
    ev->param2 = param2;
    ev->pressed = pressed;
    ev->layer = layer;
    ev->position = position;
    ev->timestamp = timestamp;
    return ev;
}
