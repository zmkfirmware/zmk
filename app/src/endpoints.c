/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/usb.h>
#include <zmk/hog.h>
#include <zmk/event-manager.h>
#include <zmk/events/usb-conn-state-changed.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum endpoint {
    ENDPOINT_USB,
    ENDPOINT_BLE,
};

static enum endpoint current_endpoint =
    COND_CODE_1(IS_ENABLED(CONFIG_ZMK_BLE), (ENDPOINT_BLE), (ENDPOINT_USB));

static int send_keypad_report() {
    struct zmk_hid_keypad_report *keypad_report = zmk_hid_get_keypad_report();

    switch (current_endpoint) {
#if IS_ENABLED(CONFIG_ZMK_USB)
    case ENDPOINT_USB: {
        int err = zmk_usb_hid_send_report((u8_t *)keypad_report, sizeof(*keypad_report));
        if (err) {
            LOG_ERR("FAILED TO SEND OVER USB: %d", err);
        }
        return err;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB) */

#if IS_ENABLED(CONFIG_ZMK_BLE)
    case ENDPOINT_BLE: {
        int err = zmk_hog_send_keypad_report(&keypad_report->body);
        if (err) {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
        return err;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_BLE) */

    default:
        LOG_ERR("Unsupported endpoint %d", current_endpoint);
        return -ENOTSUP;
    }
}

static int send_consumer_report() {
    struct zmk_hid_consumer_report *consumer_report = zmk_hid_get_consumer_report();

    switch (current_endpoint) {
#if IS_ENABLED(CONFIG_ZMK_USB)
    case ENDPOINT_USB: {
        int err = zmk_usb_hid_send_report((u8_t *)consumer_report, sizeof(*consumer_report));
        if (err) {
            LOG_ERR("FAILED TO SEND OVER USB: %d", err);
        }
        return err;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB) */

#if IS_ENABLED(CONFIG_ZMK_BLE)
    case ENDPOINT_BLE: {
        int err = zmk_hog_send_consumer_report(&consumer_report->body);
        if (err) {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
        return err;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_BLE) */

    default:
        LOG_ERR("Unsupported endpoint %d", current_endpoint);
        return -ENOTSUP;
    }
}

int zmk_endpoints_send_report(u8_t usage_page) {

    LOG_DBG("usage page 0x%02X", usage_page);
    switch (usage_page) {
    case USAGE_KEYPAD:
        return send_keypad_report();
    case USAGE_CONSUMER:
        return send_consumer_report();
    default:
        LOG_ERR("Unsupported usage page %d", usage_page);
        return -ENOTSUP;
    }
}

static bool is_usb_ready() {
#if IS_ENABLED(CONFIG_ZMK_USB)
    return zmk_usb_is_hid_ready();
#else
    return false;
#endif
}

static bool is_ble_ready() {
#if IS_ENABLED(CONFIG_ZMK_BLE)
    return zmk_ble_active_profile_is_connected();
#else
    return false;
#endif
}

static enum endpoint get_selected_endpoint() {
    if (is_ble_ready()) {
        if (is_usb_ready()) {
            LOG_DBG("Both endpoints ready. Selecting USB");
            // TODO: add user setting to control this
            return ENDPOINT_USB;
        }

        return ENDPOINT_BLE;
    }

    return ENDPOINT_USB;
}

static void disconnect_current_endpoint() {
    zmk_hid_keypad_clear();
    zmk_hid_consumer_clear();

    zmk_endpoints_send_report(USAGE_KEYPAD);
    zmk_endpoints_send_report(USAGE_CONSUMER);
}

static int endpoint_listener(const struct zmk_event_header *eh) {
    enum endpoint new_endpoint = get_selected_endpoint();

    if (new_endpoint != current_endpoint) {
        /* Cancel all current keypresses so keys don't stay held on the old endpoint. */
        disconnect_current_endpoint();

        current_endpoint = new_endpoint;
        LOG_INF("Endpoint changed: %d", current_endpoint);
    }

    return 0;
}

ZMK_LISTENER(endpoint_listener, endpoint_listener);
#if IS_ENABLED(CONFIG_USB)
ZMK_SUBSCRIPTION(endpoint_listener, usb_conn_state_changed);
#endif
// TODO: add BLE state subscription