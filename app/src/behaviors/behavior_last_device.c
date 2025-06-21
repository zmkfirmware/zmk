/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <zmk/usb_hid.h>
#include <zmk/hog.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/endpoint_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_behavior_last_device

static int8_t last_device;
static bool skip_next_endpoint_change = false;

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_DBG("Last device pressed %d", last_device);
    if (last_device == -1) {
        LOG_DBG("Toggling output");
        zmk_endpoints_toggle_transport();
    } else {
        LOG_DBG("Switching to last ble device: %d", last_device);
        zmk_ble_prof_select(last_device);
        if (zmk_endpoints_selected().transport == ZMK_TRANSPORT_USB) {
            LOG_DBG("Toggling output");
            zmk_endpoints_toggle_transport();
        }
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_last_device_init(const struct device *dev) { return 0; }

static const struct behavior_driver_api behavior_last_device_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
};

static void update_last_device_ble_index(uint8_t profile) {
    last_device = profile;
    LOG_DBG("Last device set to %d", last_device);
};

static int last_device_listener(const zmk_event_t *eh) {
    if (as_zmk_endpoint_changed(eh) != NULL) {
        if (zmk_preferred_transport() == zmk_endpoints_selected().transport) {
            if (!skip_next_endpoint_change) {
                if (zmk_endpoints_selected().transport == ZMK_TRANSPORT_USB) {
                    update_last_device_ble_index(zmk_ble_active_profile_index());
                } else {
                    last_device = -1;
                    LOG_DBG("Last device set to %d", last_device);
                }
            } else {
                skip_next_endpoint_change = false;
            }
        } else if (zmk_endpoints_selected().transport == ZMK_TRANSPORT_BLE &&
                   zmk_preferred_transport() == ZMK_TRANSPORT_USB) {
            LOG_DBG("USB disconnected");
            update_last_device_ble_index(zmk_ble_last_profile_index());
        } else {
            LOG_DBG("Skipping next endpoint change");
            skip_next_endpoint_change = true;
        }
    }

    if (as_zmk_ble_active_profile_changed(eh) != NULL &&
        zmk_endpoints_selected().transport == ZMK_TRANSPORT_BLE) {
        update_last_device_ble_index(zmk_ble_last_profile_index());
    }

    return 0;
}

ZMK_LISTENER(last_device_listener, last_device_listener);
ZMK_SUBSCRIPTION(last_device_listener, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(last_device_listener, zmk_ble_active_profile_changed);

#define LAST_DEVICE_INST(n)                                                                        \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_last_device_init, NULL, NULL, NULL, POST_KERNEL,           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_last_device_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LAST_DEVICE_INST)
