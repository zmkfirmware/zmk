/*
- Copyright (c) 2021 The ZMK Contributors
-
- SPDX-License-Identifier: MIT
*/

#pragma once

#include <zephyr/kernel.h>

#include <zmk/event_manager.h>

struct zmk_dynamic_macros_changed {
    uint8_t recording_count;
};

ZMK_EVENT_DECLARE(zmk_dynamic_macros_changed);

static inline int raise_dynamic_macros_changed(uint8_t count) {
    return raise_zmk_dynamic_macros_changed(
            (struct zmk_dynamic_macros_changed){.recording_count = count});
};
