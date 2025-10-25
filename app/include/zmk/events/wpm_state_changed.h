/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_wpm_state_changed {
    uint8_t state;
};

ZMK_EVENT_DECLARE(zmk_wpm_state_changed);
