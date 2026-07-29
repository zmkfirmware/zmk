/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifdef CONFIG_ZTEST

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/sys/ring_buffer.h>

ssize_t zmk_studio_rpc_rx_write(struct ring_buf *rpc_buf, const uint8_t *buf, uint32_t len);
void zmk_studio_gatt_rpc_rx_set_active_for_test(bool active);
ssize_t zmk_studio_gatt_rpc_rx_write_for_test(const void *buf, uint16_t len, uint16_t offset);

#endif
