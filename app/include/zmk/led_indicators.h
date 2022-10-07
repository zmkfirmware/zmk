/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/endpoints_types.h>
#include <zmk/hid.h>
#include <zmk/led_indicators_types.h>

#define ZMK_LED_INDICATORS_NUMLOCK_BIT BIT(0)
#define ZMK_LED_INDICATORS_CAPSLOCK_BIT BIT(1)
#define ZMK_LED_INDICATORS_SCROLLLOCK_BIT BIT(2)
#define ZMK_LED_INDICATORS_COMPOSE_BIT BIT(3)
#define ZMK_LED_INDICATORS_KANA_BIT BIT(4)

zmk_led_indicators_flags_t zmk_led_indicators_get_current_flags();
zmk_led_indicators_flags_t zmk_led_indicators_get_flags(enum zmk_endpoint endpoint,
                                                        uint8_t profile);
void zmk_led_indicators_update_flags(zmk_led_indicators_flags_t leds, enum zmk_endpoint endpoint,
                                     uint8_t profile);

void zmk_led_indicators_process_report(struct zmk_hid_led_report_body *report,
                                       enum zmk_endpoint endpoint, uint8_t profile);
