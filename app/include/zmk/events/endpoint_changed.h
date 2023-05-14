/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

#include <zmk/endpoints_types.h>
#include <zmk/event_manager.h>

struct zmk_endpoint_changed {
    struct zmk_endpoint_instance endpoint;
};

ZMK_EVENT_DECLARE(zmk_endpoint_changed);
