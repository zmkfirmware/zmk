/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <zmk/keys.h>
#include <zmk/hid.h>

enum zmk_usb_conn_state {
    ZMK_USB_CONN_NONE,
    ZMK_USB_CONN_POWERED,
    ZMK_USB_CONN_HID,
};

enum usb_dc_status_code zmk_usb_get_status();
enum zmk_usb_conn_state zmk_usb_get_conn_state();

static inline bool zmk_usb_is_powered() { return zmk_usb_get_conn_state() != ZMK_USB_CONN_NONE; }
static inline bool zmk_usb_is_hid_ready() { return zmk_usb_get_conn_state() == ZMK_USB_CONN_HID; }

#ifdef CONFIG_ZMK_USB
int zmk_usb_hid_send_report(const uint8_t *report, size_t len);
#endif /* CONFIG_ZMK_USB */