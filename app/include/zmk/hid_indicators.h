/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/endpoints_types.h>
#include <zmk/hid.h>
#include <zmk/hid_indicators_types.h>

zmk_hid_indicators_t zmk_hid_indicators_get_current_profile(void);
zmk_hid_indicators_t zmk_hid_indicators_get_profile(struct zmk_endpoint_instance endpoint);
void zmk_hid_indicators_set_profile(zmk_hid_indicators_t indicators,
                                    struct zmk_endpoint_instance endpoint);

void zmk_hid_indicators_process_report(struct zmk_hid_led_report_body *report,
                                       struct zmk_endpoint_instance endpoint);
