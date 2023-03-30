/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/sys_io.h>
#include <devicetree.h>

static int pinmux_puchi_ble_init(const struct device *port) {
    ARG_UNUSED(port);

#if CONFIG_BOARD_PUCHI_BLE_v1
    const struct device *p0 = device_get_binding("GPIO_0");
#if CONFIG_BOARD_PUCHI_BLE_CHARGER
    gpio_pin_configure(p0, 5, GPIO_OUTPUT);
    gpio_pin_set(p0, 5, 0);
#else
    gpio_pin_configure(p0, 5, GPIO_INPUT);
#endif
#endif
    return 0;
}

SYS_INIT(pinmux_puchi_ble_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
