/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

int zmk_input_split_report_peripheral_event(uint8_t reg, uint8_t type, uint16_t code, int32_t value,
                                            bool sync);