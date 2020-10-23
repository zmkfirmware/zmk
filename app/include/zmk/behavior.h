/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

struct zmk_behavior_binding {
    char *behavior_dev;
    u32_t param1;
    u32_t param2;
};

struct zmk_behavior_binding_event {
    int layer;
    u32_t position;
    s64_t timestamp;
};