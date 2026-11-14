/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zmk/event_manager.h>

enum zmk_led_matrix_state_change_reason {
    ZMK_LED_MATRIX_STATE_CHANGE_ENABLED = BIT(0),
    ZMK_LED_MATRIX_STATE_CHANGE_BRIGHTNESS = BIT(1),
    ZMK_LED_MATRIX_STATE_CHANGE_PROFILE = BIT(2),
    ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR = BIT(3),
    ZMK_LED_MATRIX_STATE_CHANGE_STATUS = BIT(4),
    ZMK_LED_MATRIX_STATE_CHANGE_SPEED = BIT(5),
    ZMK_LED_MATRIX_STATE_CHANGE_COLOR = BIT(6),
    ZMK_LED_MATRIX_STATE_CHANGE_CHILD_PROFILE = BIT(7),
};

struct zmk_led_matrix_state_changed {
    uint8_t reason_mask;
};

ZMK_EVENT_DECLARE(zmk_led_matrix_state_changed);
