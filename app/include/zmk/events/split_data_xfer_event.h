/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/service.h>

struct zmk_split_data_xfer_event {
    struct zmk_split_data_xfer_data data_xfer;
};

ZMK_EVENT_DECLARE(zmk_split_data_xfer_event);
