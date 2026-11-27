/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZMK_APP_SRC_LED_MATRIX_INTERNAL_H_
#define ZMK_APP_SRC_LED_MATRIX_INTERNAL_H_

enum led_matrix_runtime_mode {
    LED_MATRIX_RUNTIME_MODE_OFF = 0,
    LED_MATRIX_RUNTIME_MODE_ACTIVE,
    LED_MATRIX_RUNTIME_MODE_FORCE_SHOW_ONLY,
    LED_MATRIX_RUNTIME_MODE_AUTO_SUSPENDED,
    LED_MATRIX_RUNTIME_MODE_AUTO_SUSPENDED_FORCE_SHOW,
    LED_MATRIX_RUNTIME_MODE_SLEEP_SUSPENDED,
};

static enum led_matrix_runtime_mode led_matrix_current_runtime_mode(void);
static const char *led_matrix_runtime_mode_name(enum led_matrix_runtime_mode mode);
static bool led_matrix_runtime_mode_allows_base_render(enum led_matrix_runtime_mode mode);
static bool led_matrix_runtime_mode_allows_render(enum led_matrix_runtime_mode mode);

#endif /* ZMK_APP_SRC_LED_MATRIX_INTERNAL_H_ */
