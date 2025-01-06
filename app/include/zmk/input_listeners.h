/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/devicetree.h>

#define ZMK_INPUT_LISTENERS_UTIL_ONE(n) 1 +

#define ZMK_INPUT_LISTENERS_LEN                                                                    \
    (DT_FOREACH_STATUS_OKAY(zmk_input_listener, ZMK_INPUT_LISTENERS_UTIL_ONE) 0)
