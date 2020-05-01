/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>

#include "kscan.h"

void main(void)
{
	printk("Welcome to ZMK!\n");

	zmk_kscan_init(CONFIG_KSCAN_MATRIX_DEV_NAME);
}
