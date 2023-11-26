/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zmk/settings.h>

static int reset_settings_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return zmk_settings_erase();
}

// Reset after the kernel is initialized but before any application code to
// ensure settings are cleared before anything tries to use them.
SYS_INIT(reset_settings_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
