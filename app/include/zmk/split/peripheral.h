/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/split/transport/types.h>

int zmk_split_peripheral_report_event(const struct zmk_split_transport_peripheral_event *event);

bool zmk_split_transport_get_available(uint32_t addr);

uint32_t zmk_split_get_transport_addr_at_index(uint8_t index);