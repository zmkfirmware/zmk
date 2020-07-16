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

#ifdef CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL
#include <zmk/split/bluetooth/central.h>
#endif /* CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL */

#define ZMK_KSCAN_DEV DT_LABEL(ZMK_MATRIX_NODE_ID)

void main(void)
{
	printk("Welcome to ZMK!\n");

	if (zmk_kscan_init(ZMK_KSCAN_DEV) != 0)
	{
		return;
	}

#ifdef CONFIG_SETTINGS
	settings_load();
#endif
}
