/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZMK_APP_INCLUDE_DRIVERS_LED_LED_COMPOSITE_H_
#define ZMK_APP_INCLUDE_DRIVERS_LED_LED_COMPOSITE_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util_macro.h>

#define ZMK_LED_COMPOSITE_CAP_MANAGED_POWER BIT(0)
#define ZMK_LED_COMPOSITE_CAP_FULL_HARD_OFF BIT(1)

int zmk_led_composite_get_channel_count(const struct device *dev, uint32_t *count);
int zmk_led_composite_get_capabilities(const struct device *dev, uint32_t *caps);
/**
 * Resume the composite, or clear and shut down every child.
 *
 * Disabling succeeds only after every child has reached its available shutdown
 * state: cleared output, software blanking, or hardware power off. Failed children
 * are retried by subsequent disable calls without accessing completed children.
 * Enabling failure attempts to shut down the entire composite before returning.
 * These operations never change persisted external-power settings.
 *
 * Capability bits describe hardware facilities, not whether shutdown has completed.
 */
int zmk_led_composite_power(const struct device *dev, bool enable);

#endif /* ZMK_APP_INCLUDE_DRIVERS_LED_LED_COMPOSITE_H_ */
