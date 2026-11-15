/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/endpoint_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/*
 * A split peripheral neither selects an endpoint nor sends HID data, so these two
 * values are mirrored from the central instead of being derived locally. Only the
 * part of the endpoints API that can be answered from them is implemented here;
 * the rest is undeclared on a peripheral, see zmk/endpoints.h.
 *
 * Nothing is known until the central pushes, so the initial state is "not
 * connected" rather than a guess.
 */
static struct zmk_endpoint_instance current_instance = {.transport = ZMK_TRANSPORT_NONE};
static enum zmk_transport preferred_transport = ZMK_TRANSPORT_NONE;

enum zmk_transport zmk_endpoint_get_preferred_transport(void) { return preferred_transport; }

struct zmk_endpoint_instance zmk_endpoint_get_preferred(void) {
    struct zmk_endpoint_instance instance = {.transport = preferred_transport};

    // The active BLE profile index is only reported as part of the selected endpoint,
    // so it is only known while BLE is the selected transport.
    if (preferred_transport == ZMK_TRANSPORT_BLE &&
        current_instance.transport == ZMK_TRANSPORT_BLE) {
        instance.ble.profile_index = current_instance.ble.profile_index;
    }

    return instance;
}

struct zmk_endpoint_instance zmk_endpoint_get_selected(void) { return current_instance; }

bool zmk_endpoint_is_connected(void) { return current_instance.transport != ZMK_TRANSPORT_NONE; }

int zmk_endpoint_set_mirrored_state(struct zmk_endpoint_instance selected,
                                    enum zmk_transport preferred) {
    preferred_transport = preferred;

    if (zmk_endpoint_instance_eq(selected, current_instance)) {
        return 0;
    }

    current_instance = selected;

    char endpoint_str[ZMK_ENDPOINT_STR_LEN];
    zmk_endpoint_instance_to_str(current_instance, endpoint_str, sizeof(endpoint_str));
    LOG_INF("Endpoint changed: %s", endpoint_str);

    return raise_zmk_endpoint_changed((struct zmk_endpoint_changed){.endpoint = current_instance});
}
