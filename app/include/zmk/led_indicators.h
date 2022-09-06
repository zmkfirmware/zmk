/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/endpoints_types.h>
#include <zmk/hid.h>
#include <zmk/led_indicators_types.h>

zmk_leds_flags_t zmk_leds_get_current_flags();
zmk_leds_flags_t zmk_leds_get_flags(enum zmk_endpoint endpoint, uint8_t profile);
void zmk_leds_update_flags(zmk_leds_flags_t leds, enum zmk_endpoint endpoint, uint8_t profile);

void zmk_leds_process_report(struct zmk_hid_led_report_body *report, enum zmk_endpoint endpoint,
                             uint8_t profile);
