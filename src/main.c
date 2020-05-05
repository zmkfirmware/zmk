/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>

#include "kscan.h"
#include "usb_hid.h"

void main(void)
{
	printk("Welcome to ZMK!\n");

	if (zmk_kscan_init(CONFIG_KSCAN_MATRIX_DEV_NAME) != 0) {
		printk("Keyboard Scan Init Failed\n");
		return;
	}

	if (zmk_usb_hid_init() < 0) {
		printk("USB HID Init Failed\n");
		return;
	}
}
