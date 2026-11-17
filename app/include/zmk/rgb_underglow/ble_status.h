/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>

struct output_state {
    struct zmk_endpoint_instance selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
};

struct output_state zmk_get_output_state(void);
int zmk_rgb_underglow_set_color_ble(struct output_state os);
