/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/keys.h>
#include <zmk/hid.h>

enum zmk_endpoint {
    ZMK_ENDPOINT_USB,
    ZMK_ENDPOINT_BLE,
};

int zmk_endpoints_select(enum zmk_endpoint endpoint);
int zmk_endpoints_toggle();
enum zmk_endpoint zmk_endpoints_selected();

int zmk_endpoints_send_report(uint16_t usage_page);
