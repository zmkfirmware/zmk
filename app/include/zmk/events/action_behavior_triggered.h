/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/behavior.h>

struct zmk_action_behavior_triggered {
    const struct zmk_behavior_binding *binding;
    struct zmk_behavior_binding_event event;
    bool pressed;
};

ZMK_EVENT_DECLARE(zmk_action_behavior_triggered);

static inline int raise_action_behavior_triggered(const struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event,
                                                  bool pressed) {
    return raise_zmk_action_behavior_triggered((struct zmk_action_behavior_triggered){
        .binding = binding, .event = event, .pressed = pressed});
}