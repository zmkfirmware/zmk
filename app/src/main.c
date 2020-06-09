/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <settings/settings.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix.h>
#include <zmk/kscan.h>
#include <zmk/endpoints.h>

#define ZMK_KSCAN_DEV DT_LABEL(ZMK_MATRIX_NODE_ID)

void main(void)
{
	printk("Welcome to ZMK!\n");

	if (zmk_kscan_init(ZMK_KSCAN_DEV) != 0)
	{
		return;
	}

	if (zmk_endpoints_init())
	{
		printk("ENDPOINT INIT FAILED\n");
		return;
	}

#ifdef CONFIG_SETTINGS
	settings_load();
#endif
}
