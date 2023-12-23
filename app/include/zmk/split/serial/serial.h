/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/sys/ring_buffer.h>

// The serial protocol is defined for payloads of up to 254 bytes. This should
// be large enough to ensure that one message can be fully buffered.
#define SERIAL_BUF_SIZE 300

struct serial_device {
    const struct device *dev;
    uint8_t rx_buf[SERIAL_BUF_SIZE], tx_buf[SERIAL_BUF_SIZE];
    struct ring_buf rx_rb, tx_rb;
    struct k_work rx_work;

#ifdef CONFIG_ZMK_SPLIT_SERIAL_UART_POLL
    bool poll;
    struct k_work tx_work;
    struct k_timer rx_timer;
#endif
};

void serial_handle_rx(uint32_t cmd, uint8_t *data, uint8_t len);
