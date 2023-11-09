/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>
#include <zephyr/settings/settings.h>

#include <stdio.h>

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

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DEFAULT_TRANSPORT                                                                          \
    COND_CODE_1(IS_ENABLED(CONFIG_ZMK_BLE), (ZMK_TRANSPORT_BLE), (ZMK_TRANSPORT_USB))

static struct zmk_endpoint_instance current_instance = {};
static enum zmk_transport preferred_transport =
    ZMK_TRANSPORT_USB; /* Used if multiple endpoints are ready */

static void update_current_endpoint(void);

#if IS_ENABLED(CONFIG_SETTINGS)
static void endpoints_save_preferred_work(struct k_work *work) {
    settings_save_one("endpoints/preferred", &preferred_transport, sizeof(preferred_transport));
}

static struct k_work_delayable endpoints_save_work;
#endif

static int endpoints_save_preferred(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    return k_work_reschedule(&endpoints_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#else
    return 0;
#endif
}

bool zmk_endpoint_instance_eq(struct zmk_endpoint_instance a, struct zmk_endpoint_instance b) {
    if (a.transport != b.transport) {
        return false;
    }

    switch (a.transport) {
    case ZMK_TRANSPORT_USB:
        return true;

    case ZMK_TRANSPORT_BLE:
        return a.ble.profile_index == b.ble.profile_index;
    }

    LOG_ERR("Invalid transport %d", a.transport);
    return false;
}

int zmk_endpoint_instance_to_str(struct zmk_endpoint_instance endpoint, char *str, size_t len) {
    switch (endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        return snprintf(str, len, "USB");

    case ZMK_TRANSPORT_BLE:
        return snprintf(str, len, "BLE:%d", endpoint.ble.profile_index);

    default:
        return snprintf(str, len, "Invalid");
    }
}

#define INSTANCE_INDEX_OFFSET_USB 0
#define INSTANCE_INDEX_OFFSET_BLE ZMK_ENDPOINT_USB_COUNT

int zmk_endpoint_instance_to_index(struct zmk_endpoint_instance endpoint) {
    switch (endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        return INSTANCE_INDEX_OFFSET_USB;

    case ZMK_TRANSPORT_BLE:
        return INSTANCE_INDEX_OFFSET_BLE + endpoint.ble.profile_index;
    }

    LOG_ERR("Invalid transport %d", endpoint.transport);
    return 0;
}

int zmk_endpoints_select_transport(enum zmk_transport transport) {
    LOG_DBG("Selected endpoint transport %d", transport);

    if (preferred_transport == transport) {
        return 0;
    }

    preferred_transport = transport;

    endpoints_save_preferred();

    update_current_endpoint();

    return 0;
}

int zmk_endpoints_toggle_transport(void) {
    enum zmk_transport new_transport =
        (preferred_transport == ZMK_TRANSPORT_USB) ? ZMK_TRANSPORT_BLE : ZMK_TRANSPORT_USB;
    return zmk_endpoints_select_transport(new_transport);
}

struct zmk_endpoint_instance zmk_endpoints_selected(void) {
    return current_instance;
}

static int send_keyboard_report(void) {
    switch (current_instance.transport) {
    case ZMK_TRANSPORT_USB: {
#if IS_ENABLED(CONFIG_ZMK_USB)
        int err = zmk_usb_hid_send_keyboard_report();
        if (err) {
            LOG_ERR("FAILED TO SEND OVER USB: %d", err);
        }
        return err;
#else
        LOG_ERR("USB endpoint is not supported");
        return -ENOTSUP;
#endif /* IS_ENABLED(CONFIG_ZMK_USB) */
    }

    case ZMK_TRANSPORT_BLE: {
#if IS_ENABLED(CONFIG_ZMK_BLE)
        struct zmk_hid_keyboard_report *keyboard_report = zmk_hid_get_keyboard_report();
        int err = zmk_hog_send_keyboard_report(&keyboard_report->body);
        if (err) {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
        return err;
#else
        LOG_ERR("BLE HOG endpoint is not supported");
        return -ENOTSUP;
#endif /* IS_ENABLED(CONFIG_ZMK_BLE) */
    }
    }

    LOG_ERR("Unhandled endpoint transport %d", current_instance.transport);
    return -ENOTSUP;
}

static int send_consumer_report(void) {
    switch (current_instance.transport) {
    case ZMK_TRANSPORT_USB: {
#if IS_ENABLED(CONFIG_ZMK_USB)
        int err = zmk_usb_hid_send_consumer_report();
        if (err) {
            LOG_ERR("FAILED TO SEND OVER USB: %d", err);
        }
        return err;
#else
        LOG_ERR("USB endpoint is not supported");
        return -ENOTSUP;
#endif /* IS_ENABLED(CONFIG_ZMK_USB) */
    }

    case ZMK_TRANSPORT_BLE: {
#if IS_ENABLED(CONFIG_ZMK_BLE)
        struct zmk_hid_consumer_report *consumer_report = zmk_hid_get_consumer_report();
        int err = zmk_hog_send_consumer_report(&consumer_report->body);
        if (err) {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
        return err;
#else
        LOG_ERR("BLE HOG endpoint is not supported");
        return -ENOTSUP;
#endif /* IS_ENABLED(CONFIG_ZMK_BLE) */
    }
    }

    LOG_ERR("Unhandled endpoint transport %d", current_instance.transport);
    return -ENOTSUP;
}

int zmk_endpoints_send_report(uint16_t usage_page) {

    LOG_DBG("usage page 0x%02X", usage_page);
    switch (usage_page) {
    case HID_USAGE_KEY:
        return send_keyboard_report();

    case HID_USAGE_CONSUMER:
        return send_consumer_report();
    }

    LOG_ERR("Unsupported usage page %d", usage_page);
    return -ENOTSUP;
}

#if IS_ENABLED(CONFIG_ZMK_MOUSE)
int zmk_endpoints_send_mouse_report() {
    switch (current_instance.transport) {
    case ZMK_TRANSPORT_USB: {
#if IS_ENABLED(CONFIG_ZMK_USB)
        int err = zmk_usb_hid_send_mouse_report();
        if (err) {
            LOG_ERR("FAILED TO SEND OVER USB: %d", err);
        }
        return err;
#else
        LOG_ERR("USB endpoint is not supported");
        return -ENOTSUP;
#endif /* IS_ENABLED(CONFIG_ZMK_USB) */
    }

    case ZMK_TRANSPORT_BLE: {
#if IS_ENABLED(CONFIG_ZMK_BLE)
        struct zmk_hid_mouse_report *mouse_report = zmk_hid_get_mouse_report();
        int err = zmk_hog_send_mouse_report(&mouse_report->body);
        if (err) {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
        return err;
#else
        LOG_ERR("BLE HOG endpoint is not supported");
        return -ENOTSUP;
#endif /* IS_ENABLED(CONFIG_ZMK_BLE) */
    }
    }

    LOG_ERR("Unhandled endpoint transport %d", current_instance.transport);
    return -ENOTSUP;
}
#endif // IS_ENABLED(CONFIG_ZMK_MOUSE)

#if IS_ENABLED(CONFIG_SETTINGS)

static int endpoints_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                                void *cb_arg) {
    LOG_DBG("Setting endpoint value %s", name);

    if (settings_name_steq(name, "preferred", NULL)) {
        if (len != sizeof(enum zmk_transport)) {
            LOG_ERR("Invalid endpoint size (got %d expected %d)", len, sizeof(enum zmk_transport));
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &preferred_transport, sizeof(enum zmk_transport));
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

static bool is_usb_ready(void) {
#if IS_ENABLED(CONFIG_ZMK_USB)
    return zmk_usb_is_hid_ready();
#else
    return false;
#endif
}

static bool is_ble_ready(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE)
    return zmk_ble_active_profile_is_connected();
#else
    return false;
#endif
}

static enum zmk_transport get_selected_transport(void) {
    if (is_ble_ready()) {
        if (is_usb_ready()) {
            LOG_DBG("Both endpoint transports are ready. Using %d", preferred_transport);
            return preferred_transport;
        }

        LOG_DBG("Only BLE is ready.");
        return ZMK_TRANSPORT_BLE;
    }

    if (is_usb_ready()) {
        LOG_DBG("Only USB is ready.");
        return ZMK_TRANSPORT_USB;
    }

    LOG_DBG("No endpoint transports are ready.");
    return DEFAULT_TRANSPORT;
}

static struct zmk_endpoint_instance get_selected_instance(void) {
    struct zmk_endpoint_instance instance = {.transport = get_selected_transport()};

    switch (instance.transport) {
#if IS_ENABLED(CONFIG_ZMK_BLE)
    case ZMK_TRANSPORT_BLE:
        instance.ble.profile_index = zmk_ble_active_profile_index();
        break;
#endif // IS_ENABLED(CONFIG_ZMK_BLE)

    default:
        // No extra data for this transport.
        break;
    }

    return instance;
}

static int zmk_endpoints_init(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    int err = settings_register(&endpoints_handler);
    if (err) {
        LOG_ERR("Failed to register the endpoints settings handler (err %d)", err);
        return err;
    }

    k_work_init_delayable(&endpoints_save_work, endpoints_save_preferred_work);

    settings_load_subtree("endpoints");
#endif

    current_instance = get_selected_instance();

    return 0;
}

static void disconnect_current_endpoint(void) {
    zmk_hid_keyboard_clear();
    zmk_hid_consumer_clear();
#if IS_ENABLED(CONFIG_ZMK_MOUSE)
    zmk_hid_mouse_clear();
#endif // IS_ENABLED(CONFIG_ZMK_MOUSE)

    zmk_endpoints_send_report(HID_USAGE_KEY);
    zmk_endpoints_send_report(HID_USAGE_CONSUMER);
}

static void update_current_endpoint(void) {
    struct zmk_endpoint_instance new_instance = get_selected_instance();

    if (!zmk_endpoint_instance_eq(new_instance, current_instance)) {
        // Cancel all current keypresses so keys don't stay held on the old endpoint.
        disconnect_current_endpoint();

        current_instance = new_instance;

        char endpoint_str[ZMK_ENDPOINT_STR_LEN];
        zmk_endpoint_instance_to_str(current_instance, endpoint_str, sizeof(endpoint_str));
        LOG_INF("Endpoint changed: %s", endpoint_str);

        raise_zmk_endpoint_changed((struct zmk_endpoint_changed){.endpoint = current_instance});
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
