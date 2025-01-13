/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/devicetree.h>

#define ZMK_COMBOS_UTIL_ONE(n) +1

#define ZMK_COMBOS_LEN                                                                             \
    COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(zmk_combos),                                             \
                (0 DT_FOREACH_CHILD_STATUS_OKAY(DT_INST(0, zmk_combos), ZMK_COMBOS_UTIL_ONE)),     \
                (0))

int zmk_combo_trigger_behavior_invoked(int trigger_id, char *fallback_behavior_dev,
                                       uint32_t fallback_param,
                                       struct zmk_behavior_binding_event event, bool state);