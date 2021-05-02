/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <init.h>
#include <settings/settings.h>

#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <zmk/usb.h>
#include <zmk/hog.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/endpoint_selection_changed.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DEFAULT_ENDPOINT                                                                           \
    COND_CODE_1(IS_ENABLED(CONFIG_ZMK_BLE), (ZMK_ENDPOINT_BLE), (ZMK_ENDPOINT_USB))

static enum zmk_endpoint current_endpoint = DEFAULT_ENDPOINT;
static enum zmk_endpoint preferred_endpoint =
    ZMK_ENDPOINT_USB; /* Used if multiple endpoints are ready */

static void update_current_endpoint();

#if IS_ENABLED(CONFIG_SETTINGS)
static void endpoints_save_preferred_work(struct k_work *work) {
    settings_save_one("endpoints/preferred", &preferred_endpoint, sizeof(preferred_endpoint));
}

static struct k_delayed_work endpoints_save_work;
#endif

static int endpoints_save_preferred() {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_delayed_work_cancel(&endpoints_save_work);
    return k_delayed_work_submit(&endpoints_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#else
    return 0;
#endif
}

int zmk_endpoints_select(enum zmk_endpoint endpoint) {
    LOG_DBG("Selected endpoint %d", endpoint);

    if (preferred_endpoint == endpoint) {
        return 0;
    }

    preferred_endpoint = endpoint;

    endpoints_save_preferred();

    update_current_endpoint();

    return 0;
}

enum zmk_endpoint zmk_endpoints_selected() { return current_endpoint; }

int zmk_endpoints_toggle() {
    enum zmk_endpoint new_endpoint =
        (preferred_endpoint == ZMK_ENDPOINT_USB) ? ZMK_ENDPOINT_BLE : ZMK_ENDPOINT_USB;
    return zmk_endpoints_select(new_endpoint);
}

static int send_keyboard_report() {
    struct zmk_hid_keyboard_report *keyboard_report = zmk_hid_get_keyboard_report();

    switch (current_endpoint) {
#if IS_ENABLED(CONFIG_ZMK_USB)
    case ZMK_ENDPOINT_USB: {
        int err = zmk_usb_hid_send_report((uint8_t *)keyboard_report, sizeof(*keyboard_report));
        if (err) {
            LOG_ERR("FAILED TO SEND OVER USB: %d", err);
        }
        return err;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB) */

#if IS_ENABLED(CONFIG_ZMK_BLE)
    case ZMK_ENDPOINT_BLE: {
        int err = zmk_hog_send_keyboard_report(&keyboard_report->body);
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
    case ZMK_ENDPOINT_USB: {
        int err = zmk_usb_hid_send_report((uint8_t *)consumer_report, sizeof(*consumer_report));
        if (err) {
            LOG_ERR("FAILED TO SEND OVER USB: %d", err);
        }
        return err;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB) */

#if IS_ENABLED(CONFIG_ZMK_BLE)
    case ZMK_ENDPOINT_BLE: {
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

int zmk_endpoints_send_report(uint16_t usage_page) {

    LOG_DBG("usage page 0x%02X", usage_page);
    switch (usage_page) {
    case HID_USAGE_KEY:
        return send_keyboard_report();
    case HID_USAGE_CONSUMER:
        return send_consumer_report();
    default:
        LOG_ERR("Unsupported usage page %d", usage_page);
        return -ENOTSUP;
    }
}

int zmk_endpoints_send_mouse_report() {
    LOG_ERR("SENDING MOUSE REPORT");
    struct zmk_hid_mouse_report *mouse_report = zmk_hid_get_mouse_report();

    switch (current_endpoint) {
#if IS_ENABLED(CONFIG_ZMK_USB)
        case ZMK_ENDPOINT_USB: {
        int err = zmk_usb_hid_send_report((uint8_t *)mouse_report, sizeof(*mouse_report));
        if (err) {
            LOG_ERR("FAILED TO SEND OVER USB: %d", err);
        }
        return err;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB) */

#if IS_ENABLED(CONFIG_ZMK_BLE)
        case ZMK_ENDPOINT_BLE: {
        int err = zmk_hog_send_mouse_report(&mouse_report->body);
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

#if IS_ENABLED(CONFIG_SETTINGS)

static int endpoints_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                                void *cb_arg) {
    LOG_DBG("Setting endpoint value %s", log_strdup(name));

    if (settings_name_steq(name, "preferred", NULL)) {
        if (len != sizeof(enum zmk_endpoint)) {
            LOG_ERR("Invalid endpoint size (got %d expected %d)", len, sizeof(enum zmk_endpoint));
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &preferred_endpoint, sizeof(enum zmk_endpoint));
        if (err <= 0) {
            LOG_ERR("Failed to read preferred endpoint from settings (err %d)", err);
            return err;
        }

        update_current_endpoint();
    }

    return 0;
}

struct settings_handler endpoints_handler = {.name = "endpoints", .h_set = endpoints_handle_set};
#endif /* IS_ENABLED(CONFIG_SETTINGS) */

static int zmk_endpoints_init(const struct device *_arg) {
#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    int err = settings_register(&endpoints_handler);
    if (err) {
        LOG_ERR("Failed to register the endpoints settings handler (err %d)", err);
        return err;
    }

    k_delayed_work_init(&endpoints_save_work, endpoints_save_preferred_work);

    settings_load_subtree("endpoints");
#endif

    return 0;
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

static enum zmk_endpoint get_selected_endpoint() {
    if (is_ble_ready()) {
        if (is_usb_ready()) {
            LOG_DBG("Both endpoints are ready. Using %d", preferred_endpoint);
            return preferred_endpoint;
        }

        LOG_DBG("Only BLE is ready.");
        return ZMK_ENDPOINT_BLE;
    }

    if (is_usb_ready()) {
        LOG_DBG("Only USB is ready.");
        return ZMK_ENDPOINT_USB;
    }

    LOG_DBG("No endpoints are ready.");
    return DEFAULT_ENDPOINT;
}

static void disconnect_current_endpoint() {
    zmk_hid_keyboard_clear();
    zmk_hid_consumer_clear();
    zmk_hid_mouse_clear();

    zmk_endpoints_send_report(HID_USAGE_KEY);
    zmk_endpoints_send_report(HID_USAGE_CONSUMER);
}

static void update_current_endpoint() {
    enum zmk_endpoint new_endpoint = get_selected_endpoint();

    if (new_endpoint != current_endpoint) {
        /* Cancel all current keypresses so keys don't stay held on the old endpoint. */
        disconnect_current_endpoint();

        current_endpoint = new_endpoint;
        LOG_INF("Endpoint changed: %d", current_endpoint);

        ZMK_EVENT_RAISE(new_zmk_endpoint_selection_changed(
            (struct zmk_endpoint_selection_changed){.endpoint = current_endpoint}));
    }
}

static int endpoint_listener(const zmk_event_t *eh) {
    update_current_endpoint();
    return 0;
}

ZMK_LISTENER(endpoint_listener, endpoint_listener);
#if IS_ENABLED(CONFIG_ZMK_USB)
ZMK_SUBSCRIPTION(endpoint_listener, zmk_usb_conn_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(endpoint_listener, zmk_ble_active_profile_changed);
#endif

SYS_INIT(zmk_endpoints_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
