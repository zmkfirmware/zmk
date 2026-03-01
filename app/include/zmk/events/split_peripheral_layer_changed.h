/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_split_peripheral_layer_changed {
    uint32_t layers;
};

ZMK_EVENT_DECLARE(zmk_split_peripheral_layer_changed);
