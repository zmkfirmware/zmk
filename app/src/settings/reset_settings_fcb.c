/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

int zmk_settings_erase(void) {
    LOG_ERR("Settings reset is not implemented for deprecated FCB backend.");
    return -ENOSYS;
}
