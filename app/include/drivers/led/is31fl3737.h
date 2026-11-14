/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZMK_APP_INCLUDE_DRIVERS_LED_IS31FL3737_H_
#define ZMK_APP_INCLUDE_DRIVERS_LED_IS31FL3737_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

int is31fl3737_blank(const struct device *dev, bool blank_en);
/* Restore registers and the driver's page cache after hardware shutdown. */
int is31fl3737_resume(const struct device *dev);
int is31fl3737_current_limit(const struct device *dev, uint8_t limit);
int is31fl3737_write_abm_modes(const struct device *dev, uint32_t start_channel,
                               uint32_t num_channels, const uint8_t *buf);
int is31fl3737_configure_abm_bank(const struct device *dev, uint8_t bank, const uint8_t regs[4]);
int is31fl3737_enable_abm(const struct device *dev, bool enable);

#endif /* ZMK_APP_INCLUDE_DRIVERS_LED_IS31FL3737_H_ */
