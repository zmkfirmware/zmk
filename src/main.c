/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>

#include "zmk.h"
#include "kscan.h"
#include "usb_hid.h"

#define ZMK_KSCAN_DEV DT_LABEL(ZMK_MATRIX_NODE_ID)
void main(void)
{
	printk("Welcome to ZMK!\n");

	if (zmk_kscan_init(ZMK_KSCAN_DEV) != 0) {
		printk("Keyboard Scan Init Failed\n");
		return;
	}

	if (zmk_usb_hid_init() < 0) {
		printk("USB HID Init Failed\n");
		return;
	}
}
