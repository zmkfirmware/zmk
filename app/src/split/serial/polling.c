/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "private.h"

static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_split_serial));

void zmk_split_serial_send(const void *const data_, const size_t length) {
    const uint8_t *const data = data_;
    for (size_t position = 0; position < length; position += 1) {
        uart_poll_out(uart_dev, data[position]);
    }
}

static int init(const struct device *dev) {
    ARG_UNUSED(dev);

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("split uart device is not ready");
        return -EAGAIN;
    }

    return 0;
}
SYS_INIT(init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
