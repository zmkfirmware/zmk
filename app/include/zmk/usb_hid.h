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

int zmk_usb_hid_init();

enum usb_dc_status_code zmk_usb_hid_get_status();

int zmk_usb_hid_send_report(u8_t *report, size_t len);
