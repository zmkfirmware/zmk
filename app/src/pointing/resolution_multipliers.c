/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/pointing/resolution_multipliers.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_pointing_resolution_multipliers multipliers[ZMK_ENDPOINT_COUNT] = {
    [0 ... ZMK_ENDPOINT_COUNT - 1] =
        {
            .wheel = 15,
            .hor_wheel = 15,
        },
};

struct zmk_pointing_resolution_multipliers
zmk_pointing_resolution_multipliers_get_current_profile(void) {
    return zmk_pointing_resolution_multipliers_get_profile(zmk_endpoints_selected());
}

struct zmk_pointing_resolution_multipliers
zmk_pointing_resolution_multipliers_get_profile(struct zmk_endpoint_instance endpoint) {
    const int profile = zmk_endpoint_instance_to_index(endpoint);
    return multipliers[profile];
}

void zmk_pointing_resolution_multipliers_set_profile(struct zmk_pointing_resolution_multipliers m,
                                                     struct zmk_endpoint_instance endpoint) {
    int profile = zmk_endpoint_instance_to_index(endpoint);

    // This write is not happening on the main thread. To prevent potential data races, every
    // operation involving hid_indicators must be atomic. Currently, each function either reads
    // or writes only one entry at a time, so it is safe to do these operations without a lock.
    multipliers[profile] = m;
}

void zmk_pointing_resolution_multipliers_process_report(
    struct zmk_hid_mouse_resolution_feature_report_body *report,
    struct zmk_endpoint_instance endpoint) {
    struct zmk_pointing_resolution_multipliers vals = {
        .wheel = report->wheel_res,
        .hor_wheel = report->hwheel_res,
    };
    zmk_pointing_resolution_multipliers_set_profile(vals, endpoint);

    LOG_DBG("Update resolution multipliers: endpoint=%d, wheel=%d, hor_wheel=%d",
            endpoint.transport, vals.wheel, vals.hor_wheel);
}
