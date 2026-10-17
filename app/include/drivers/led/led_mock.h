/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZMK_APP_INCLUDE_DRIVERS_LED_LED_MOCK_H_
#define ZMK_APP_INCLUDE_DRIVERS_LED_LED_MOCK_H_

#include <stdbool.h>

#include <zephyr/device.h>

enum zmk_led_mock_managed_power_kind {
    ZMK_LED_MOCK_MANAGED_POWER_NONE = 0,
    ZMK_LED_MOCK_MANAGED_POWER_IS31FL3733,
    ZMK_LED_MOCK_MANAGED_POWER_IS31FL3737,
};

int zmk_led_mock_blank(const struct device *dev, bool blank_en);
int zmk_led_mock_reinit(const struct device *dev);
enum zmk_led_mock_managed_power_kind zmk_led_mock_managed_power_kind(const struct device *dev);

#endif /* ZMK_APP_INCLUDE_DRIVERS_LED_LED_MOCK_H_ */
