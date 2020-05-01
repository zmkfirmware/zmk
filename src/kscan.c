/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/kscan.h>

static void zmk_kscan_callback(struct device *dev, u32_t row, u32_t column, bool pressed) {
	printk("Row: %d, col: %d, pressed: %s\n", row, column, (pressed ? "true" : "false"));
	// TODO: Push this to a message box, and then trigger a work item!
}

int zmk_kscan_init(char* name) {
	struct device *dev = device_get_binding(name);
	if (dev == NULL) {
		printk("NO DEVICE!\n");
		return -EINVAL;
	}

	return 0;

	kscan_config(dev, zmk_kscan_callback);

	kscan_enable_callback(dev);
}
