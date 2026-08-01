/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define ZMK_BEHAVIOR_OMIT(_name)                                                                   \
    !(defined(ZMK_BEHAVIORS_KEEP_##_name) ||                                                       \
      (defined(ZMK_BEHAVIORS_KEEP_ALL) && !defined(ZMK_BEHAVIORS_OMIT_##_name)))

#define PLACEHOLDER 0
#define BINDING_PARAM(arg1, arg2) ((arg1 << 2) | arg2)