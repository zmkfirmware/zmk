/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/event_manager.h>

struct zmk_underglow_color_changed {};

ZMK_EVENT_DECLARE(zmk_underglow_color_changed);
