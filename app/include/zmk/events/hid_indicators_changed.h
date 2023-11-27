/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/hid_indicators_types.h>
#include <zmk/event_manager.h>

struct zmk_hid_indicators_changed {
    zmk_hid_indicators_t indicators;
};

ZMK_EVENT_DECLARE(zmk_hid_indicators_changed);
