/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/endpoints_types.h>

int zmk_endpoints_select(enum zmk_endpoint endpoint);
int zmk_endpoints_toggle();
enum zmk_endpoint zmk_endpoints_selected();

int zmk_endpoints_send_report(uint16_t usage_page);
