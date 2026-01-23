/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/toolchain.h>
#include <dt-bindings/zmk/reset.h>

/**
 * Reboot the system.
 * @param type If CONFIG_RETENTION_BOOT_MODE is set: A BOOT_MODE_TYPE_* value indicating which type
 * of reboot. Otherwise, A ZMK_RESET_* value indicating how to reboot.
 */
void zmk_reset(int type);

/**
 * Clear all persistent settings.
 *
 * This should typically be followed by a call to zmk_reset() to ensure that
 * all subsystems are properly reset.
 */
void zmk_reset_settings(void);
