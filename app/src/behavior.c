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

int zmk_behavior_state_changed(const struct behavior_state_changed *ev) {
    int ret;
    if (ev->pressed) {
        ret = behavior_keymap_binding_pressed(ev);
    } else {
        ret = behavior_keymap_binding_released(ev);
    }
    if (ret < 0) {
        return ret;
    }
    switch (ret) {
    case ZMK_BEHAVIOR_TRANSPARENT:
        return ZMK_EV_EVENT_BUBBLE;
    case ZMK_BEHAVIOR_OPAQUE:
        return ZMK_EV_EVENT_HANDLED;
    case ZMK_BEHAVIOR_CAPTURED:
        return ZMK_EV_EVENT_CAPTURED;
    default:
        return -ENOTSUP;
    }
}

int behavior_listener(const struct zmk_event_header *eh) {
    if (is_behavior_state_changed(eh)) {
        const struct behavior_state_changed *ev = cast_behavior_state_changed(eh);
        return zmk_behavior_state_changed(ev);
    }
    return 0;
}

ZMK_LISTENER(behavior, behavior_listener);
ZMK_SUBSCRIPTION(behavior, behavior_state_changed);