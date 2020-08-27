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
#include <zmk/display.h>
#include <zmk/battery.h>

#define ZMK_KSCAN_DEV DT_LABEL(ZMK_MATRIX_NODE_ID)

void main(void)
{
	LOG_INF("Welcome to ZMK!\n");

	if (zmk_kscan_init(ZMK_KSCAN_DEV) != 0)
	{
		return;
	}

#ifdef CONFIG_ZMK_DISPLAY
	zmk_display_init();

	while (1) {
		zmk_display_task_handler();
	}
#endif /* CONFIG_ZMK_DISPLAY */

#ifdef CONFIG_ZMK_BATTERY
    if (zmk_log_battery_enable() != 0)
    {
        LOG_ERR("Could not enable battery logging\n");
        return;
    }
    zmk_log_battery_sample();
    zmk_log_battery_disable();
#endif /* CONFIG_ZMK_BATTERY */
}
