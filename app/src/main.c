/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix.h>
#include <zmk/kscan.h>
#include <zmk/display.h>
#include <drivers/ext_power.h>

int main(void) {
    LOG_INF("Welcome to ZMK!\n");

    if (zmk_kscan_init(DEVICE_DT_GET(ZMK_MATRIX_NODE_ID)) != 0) {
        return -ENOTSUP;
    }

#ifdef CONFIG_ZMK_DISPLAY
    zmk_display_init();
#endif /* CONFIG_ZMK_DISPLAY */

    return 0;
}
