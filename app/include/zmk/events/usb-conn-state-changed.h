/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <usb/usb_device.h>

#include <zmk/event-manager.h>
#include <zmk/usb.h>

struct usb_conn_state_changed {
    struct zmk_event_header header;
    enum zmk_usb_conn_state conn_state;
};

ZMK_EVENT_DECLARE(usb_conn_state_changed);