/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zmk/split/serial/serial.h>

// TODO TODO TODO
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(slicemk);

// TODO TODO TODO these two should be in a header somewhere

// TODO TODO TODO implement central to peripheral data transfer
void serial_handle_rx(uint32_t cmd, uint8_t *data, uint8_t len) {
    LOG_HEXDUMP_ERR(data, len, "central to peripheral");
}

void send_position_state_impl(uint8_t *state, int len) {
    serial_write_uart(0x73627400, state, len);
}
