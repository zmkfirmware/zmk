/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <usb/usb_device.h>

#include <zmk/event_manager.h>
#include <zmk/usb.h>

struct zmk_usb_conn_state_changed {
    enum zmk_usb_conn_state conn_state;
};

ZMK_EVENT_DECLARE(zmk_usb_conn_state_changed);