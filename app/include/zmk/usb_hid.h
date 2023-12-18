/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

int zmk_usb_hid_send_keyboard_report(void);
int zmk_usb_hid_send_consumer_report(void);
#if IS_ENABLED(CONFIG_ZMK_MOUSE)
int zmk_usb_hid_send_mouse_report(void);
#endif // IS_ENABLED(CONFIG_ZMK_MOUSE)
void zmk_usb_hid_set_protocol(uint8_t protocol);
