/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>

struct zmk_split_peripheral_status_changed {
    bool connected;
};

ZMK_EVENT_DECLARE(zmk_split_peripheral_status_changed);
