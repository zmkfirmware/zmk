/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

#if IS_ENABLED(CONFIG_ZMK_MOUSE)
int zmk_mouse_usb_hid_send_mouse_report(void);
#endif // IS_ENABLED(CONFIG_ZMK_MOUSE)
