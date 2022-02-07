/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <init.h>

/* Caller does/should not need to free `data`
 * Data will be freed immediately after calling this callback */
typedef int (*rx_complete_t)(const uint8_t *data, size_t length);

void split_serial_async_init(rx_complete_t complete_fn);

void split_serial_async_send(uint8_t *data, size_t length);

uint8_t *alloc_split_serial_buffer(k_timeout_t timeout);

void free_split_serial_buffer(const uint8_t *data);
