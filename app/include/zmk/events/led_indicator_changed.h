/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/led_indicators_types.h>
#include <zmk/event_manager.h>

struct zmk_led_changed {
    zmk_leds_flags_t leds;
};

ZMK_EVENT_DECLARE(zmk_led_changed);
