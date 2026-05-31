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
