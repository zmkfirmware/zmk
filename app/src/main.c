/*
 * Copyright (c) 2020 The ZMK Contributors
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
#include <drivers/ext_power.h>

#define ZMK_KSCAN_DEV DT_LABEL(ZMK_MATRIX_NODE_ID)

void main(void) {
    LOG_INF("Welcome to ZMK!\n");

    if (zmk_kscan_init(ZMK_KSCAN_DEV) != 0) {
        return;
    }

#ifdef CONFIG_ZMK_DISPLAY
    zmk_display_init();
#endif /* CONFIG_ZMK_DISPLAY */
}
