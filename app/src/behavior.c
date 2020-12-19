/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <sys/util.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix.h>
#include <zmk/sensors.h>
#include <zmk/keymap.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/behavior_state_changed.h>
#include <zmk/events/sensor_event.h>

int zmk_behavior_state_changed_listener(const zmk_event_t *zmk_ev) {
    struct zmk_behavior_state_changed *ev = as_zmk_behavior_state_changed(zmk_ev);
    if (ev == NULL) {
        return 0;
    }
    const struct zmk_behavior_binding_event event = {
        .layer = ev->layer,
        .position = ev->position,
        .timestamp = ev->timestamp,
    };

    int err = behavior_keymap_binding_convert_central_state_dependent_params(&ev->binding, event);
    if (err) {
        LOG_ERR("Failed to convert relative to absolute behavior binding (err %d)", err);
        return err;
    }

    int ret;
    if (ev->pressed) {
        ret = behavior_keymap_binding_pressed(&ev->binding, event);
    } else {
        ret = behavior_keymap_binding_released(&ev->binding, event);
    }
    if (ret < 0) {
        return ret;
    }
    switch (ret) {
    case ZMK_BEHAVIOR_TRANSPARENT:
        return ZMK_EV_EVENT_BUBBLE;
    case ZMK_BEHAVIOR_OPAQUE:
        return ZMK_EV_EVENT_HANDLED;
    default:
        return -ENOTSUP;
    }
}

ZMK_LISTENER(behavior, zmk_behavior_state_changed_listener);
ZMK_SUBSCRIPTION(behavior, zmk_behavior_state_changed);