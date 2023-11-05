/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int display_event_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (!ev) {
        return 0;
    }

    switch (ev->state) {
    case ZMK_ACTIVITY_ACTIVE:
        zmk_display_blanking_off();
        return 0;

    case ZMK_ACTIVITY_IDLE:
    case ZMK_ACTIVITY_SLEEP:
        zmk_display_blanking_on();
        return 0;

    default:
        LOG_WRN("Unhandled activity state: %d", ev->state);
    }

    return -EINVAL;
}

ZMK_LISTENER(display_idle, display_event_handler);
ZMK_SUBSCRIPTION(display_idle, zmk_activity_state_changed);
