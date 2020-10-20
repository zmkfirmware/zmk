/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_split_listener

#include <device.h>
#include <power/reboot.h>
#include <logging/log.h>

#include <zmk/split/bluetooth/service.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event-manager.h>
#include <zmk/events/position-state-changed.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>

int split_listener(const struct zmk_event_header *eh) {
    LOG_DBG("");
    if (is_position_state_changed(eh)) {
        const struct position_state_changed *ev = cast_position_state_changed(eh);
        if (ev->state) {
            return zmk_split_bt_position_pressed(ev->position);
        } else {
            return zmk_split_bt_position_released(ev->position);
        }
    }
    return 0;
}

ZMK_LISTENER(split_listener, split_listener);
ZMK_SUBSCRIPTION(split_listener, position_state_changed);