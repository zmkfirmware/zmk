/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/split/transport/types.h>

int zmk_split_peripheral_report_event(const struct zmk_split_transport_peripheral_event *event);
