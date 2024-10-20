/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

/**
 * Erases all saved settings.
 *
 * @note This does not automatically update any code using Zephyr's settings
 * subsystem. This should typically be followed by a call to sys_reboot().
 */
int zmk_settings_erase(void);
