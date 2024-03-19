/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>

#include <zmk/settings.h>

// Reset after the kernel is initialized but before any application code to
// ensure settings are cleared before anything tries to use them.
SYS_INIT(zmk_settings_erase, POST_KERNEL, CONFIG_ZMK_SETTINGS_RESET_ON_START_INIT_PRIORITY);
