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
 * @param type A ZMK_RESET_* value indicating how to reboot.
 */
FUNC_NORETURN void zmk_reset(int type);
