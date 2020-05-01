/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/kscan.h>


void zmk_kscan_callback(struct device *dev, u32_t row, u32_t column, bool pressed) {
	printk("Row: %d, col: %d, pressed: %s\n", row, column, (pressed ? "true" : "false"));
}


void main(void)
{
	struct device *dev;
	printk("Welcome to ZMK!\n");

	dev = device_get_binding(CONFIG_KSCAN_MATRIX_DEV_NAME);
	if (dev == NULL) {
		printk("NO DEVICE!\n");
		return;
	}

	kscan_config(dev, zmk_kscan_callback);

	kscan_enable_callback(dev);
}
