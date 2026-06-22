/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zephyr/sys/util.h>

typedef uint16_t zmk_behavior_local_id_t;

struct zmk_behavior_binding {
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS_IN_BINDINGS)
    zmk_behavior_local_id_t local_id;
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS_IN_BINDINGS)
    const char *behavior_dev;
    uint32_t param1;
    uint32_t param2;
};

enum zmk_behavior_trigger_type {
    ZMK_BEHAVIOR_TRIG_TYPE_PRESS,
    ZMK_BEHAVIOR_TRIG_TYPE_RELEASE,
    ZMK_BEHAVIOR_TRIG_TYPE_SENSOR,
};

struct zmk_behavior_binding_event {
    int64_t timestamp;
    const char *behavior_dev;
    uint32_t param1;
    uint32_t param2;
    uint32_t position;
    enum zmk_behavior_trigger_type type;
    uint8_t layer;
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    uint8_t source;
#endif
};

ZMK_EVENT_DECLARE(zmk_behavior_binding_event);

static inline int
reraise_behavior_binding_event_with_new_binding(const struct zmk_behavior_binding *new_binding,
                                                struct zmk_behavior_binding_event *event) {
    return raise_zmk_behavior_binding_event((struct zmk_behavior_binding_event){
        .timestamp = event->timestamp,
        .behavior_dev = new_binding->behavior_dev,
        .param1 = new_binding->param1,
        .param2 = new_binding->param2,
        .position = event->position,
        .type = event->type,
        .layer = event->layer,
    });
}
