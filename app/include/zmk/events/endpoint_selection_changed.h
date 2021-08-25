/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>

#include <zmk/endpoints_types.h>
#include <zmk/event_manager.h>

struct zmk_endpoint_selection_changed {
    enum zmk_endpoint endpoint;
};

ZMK_EVENT_DECLARE(zmk_endpoint_selection_changed);
