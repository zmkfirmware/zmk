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
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/endpoint_changed.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Name of the subtree for endpoint-related settings
#define SETTING_SUBTREE "endpoints"

// Key of the setting to store the preferred_transport value.
#define SETTING_PREFERRED_TRANSPORT_KEY "preferred2"
// Full name of the setting to store the preferred_transport value.
#define SETTING_PREFERRED_TRANSPORT SETTING_SUBTREE "/" SETTING_PREFERRED_TRANSPORT_KEY

// Key of the deprecated setting which stored preferred_transport with an older type.
#define SETTING_PREFERRED_TRANSPORT_V1_KEY "preferred"
// Full name of the deprecated setting which stored preferred_transport with an older type.
#define SETTING_PREFERRED_TRANSPORT_V1 SETTING_SUBTREE "/" SETTING_PREFERRED_TRANSPORT_V1_KEY

#if IS_ENABLED(CONFIG_ZMK_USB)
#define DEFAULT_TRANSPORT ZMK_TRANSPORT_USB
#elif IS_ENABLED(CONFIG_ZMK_BLE)
#define DEFAULT_TRANSPORT ZMK_TRANSPORT_BLE
#else
#define DEFAULT_TRANSPORT ZMK_TRANSPORT_NONE
#endif

static struct zmk_endpoint_instance current_instance = {};

// Transport to use if multiple endpoints are ready
static enum zmk_transport preferred_transport = DEFAULT_TRANSPORT;

static void update_current_endpoint(void);

#if IS_ENABLED(CONFIG_SETTINGS)
static void endpoints_save_preferred_work(struct k_work *work) {
    settings_save_one(SETTING_PREFERRED_TRANSPORT, &preferred_transport,
                      sizeof(preferred_transport));
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
    case ZMK_TRANSPORT_NONE:
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
    case ZMK_TRANSPORT_NONE:
        return snprintf(str, len, "None");

    case ZMK_TRANSPORT_USB:
        return snprintf(str, len, "USB");

    case ZMK_TRANSPORT_BLE:
        return snprintf(str, len, "BLE:%d", endpoint.ble.profile_index);

    default:
        return snprintf(str, len, "Invalid");
    }
}

#define INSTANCE_INDEX_OFFSET_NONE 0
#define INSTANCE_INDEX_OFFSET_USB (INSTANCE_INDEX_OFFSET_NONE + ZMK_ENDPOINT_NONE_COUNT)
#define INSTANCE_INDEX_OFFSET_BLE (INSTANCE_INDEX_OFFSET_USB + ZMK_ENDPOINT_USB_COUNT)

int zmk_endpoint_instance_to_index(struct zmk_endpoint_instance endpoint) {
    switch (endpoint.transport) {
    case ZMK_TRANSPORT_NONE:
        return INSTANCE_INDEX_OFFSET_NONE;

    case ZMK_TRANSPORT_USB:
        return INSTANCE_INDEX_OFFSET_USB;

    case ZMK_TRANSPORT_BLE:
        return INSTANCE_INDEX_OFFSET_BLE + endpoint.ble.profile_index;
    }

    LOG_ERR("Invalid transport %d", endpoint.transport);
    return 0;
}

int zmk_endpoint_set_preferred_transport(enum zmk_transport transport) {
    LOG_DBG("Selected endpoint transport %d", transport);

    if (preferred_transport == transport) {
        return 0;
    }

    preferred_transport = transport;

    endpoints_save_preferred();

    update_current_endpoint();

    return 0;
}

enum zmk_transport zmk_endpoint_get_preferred_transport(void) { return preferred_transport; }

int zmk_endpoint_toggle_preferred_transport(void) {
    enum zmk_transport new_transport =
        (preferred_transport == ZMK_TRANSPORT_USB) ? ZMK_TRANSPORT_BLE : ZMK_TRANSPORT_USB;
    return zmk_endpoint_set_preferred_transport(new_transport);
}

static struct zmk_endpoint_instance get_instance_from_transport(enum zmk_transport transport) {
    struct zmk_endpoint_instance instance = {.transport = transport};
    switch (instance.transport) {
    case ZMK_TRANSPORT_BLE:
#if IS_ENABLED(CONFIG_ZMK_BLE)
        instance.ble.profile_index = zmk_ble_active_profile_index();
#endif // IS_ENABLED(CONFIG_ZMK_BLE)
        break;

    default:
        // No extra data for this transport.
        break;
    }

    return instance;
}

struct zmk_endpoint_instance zmk_endpoint_get_preferred(void) {
    return get_instance_from_transport(preferred_transport);
}

struct zmk_endpoint_instance zmk_endpoint_get_selected(void) { return current_instance; }

bool zmk_endpoint_is_connected(void) { return current_instance.transport != ZMK_TRANSPORT_NONE; }

static int send_keyboard_report(void) {
    switch (current_instance.transport) {
    case ZMK_TRANSPORT_NONE:
        return 0;

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
    case ZMK_TRANSPORT_NONE:
        return 0;

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

int zmk_endpoint_send_report(uint16_t usage_page) {
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

#if IS_ENABLED(CONFIG_ZMK_POINTING)
int zmk_endpoint_send_mouse_report() {
    switch (current_instance.transport) {
    case ZMK_TRANSPORT_NONE:
        return 0;

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
#endif // IS_ENABLED(CONFIG_ZMK_POINTING)

#if IS_ENABLED(CONFIG_SETTINGS)

// Type for the deprecated SETTING_PREFERRED_TRANSPORT_V1 setting. To maintain backwards
// compatibility when ZMK_TRANSPORT_NONE was inserted into the beginning of enum zmk_transport, the
// setting was moved to SETTING_PREFERRED_TRANSPORT. If the deprecated setting exists, it must be
// upgraded to the new type and setting name.
enum transport_v1 {
    TRANSPORT_V1_USB = 0,
    TRANSPORT_V1_BLE = 1,
};

static enum zmk_transport upgrade_transport_v1(enum transport_v1 value) {
    switch (value) {
    case TRANSPORT_V1_USB:
        return ZMK_TRANSPORT_USB;

    case TRANSPORT_V1_BLE:
        return ZMK_TRANSPORT_BLE;
    };

    LOG_ERR("Invalid transport_v1 value: %d", value);
    return ZMK_TRANSPORT_USB;
}

/**
 * Loads the deprecated SETTING_PREFERRED_TRANSPORT_V1 setting and upgrades it to the new type,
 * storing the value in SETTING_PREFERRED_TRANSPORT and deleting the original setting.
 */
static int endpoint_settings_load_preferred_v1(size_t len, settings_read_cb read_cb, void *cb_arg) {
    enum transport_v1 value;

    if (len != sizeof(value)) {
        LOG_ERR("Invalid zmk_transport size (got %zu expected %zu)", len, sizeof(value));
        return -EINVAL;
    }

    ssize_t len_read = read_cb(cb_arg, &value, sizeof(value));
    if (len_read < 0) {
        LOG_ERR("Failed to read preferred endpoint v1 from settings (err %d)", len_read);
        return len_read;
    }

    preferred_transport = upgrade_transport_v1(value);

    int err = settings_delete(SETTING_PREFERRED_TRANSPORT_V1);
    if (err != 0) {
        LOG_ERR("Failed to delete preferred endpoint v1 setting (err %d)", err);
        return err;
    }

    err = settings_save_one(SETTING_PREFERRED_TRANSPORT, &preferred_transport,
                            sizeof(preferred_transport));
    if (err == 0) {
        LOG_INF("Upgraded preferred endpoint setting");
    } else {
        LOG_ERR("Failed to save upgraded endpoint value (err %d)", err);
    }

    return err;
}

/**
 * Loads the SETTING_PREFERRED_TRANSPORT setting.
 */
static int endpoint_settings_load_preferred_v2(size_t len, settings_read_cb read_cb, void *cb_arg) {
    if (len != sizeof(preferred_transport)) {
        LOG_ERR("Invalid zmk_transport size (got %zu expected %zu)", len,
                sizeof(preferred_transport));
        return -EINVAL;
    }

    ssize_t len_read = read_cb(cb_arg, &preferred_transport, sizeof(preferred_transport));
    if (len_read < 0) {
        LOG_ERR("Failed to read preferred endpoint from settings (err %d)", len_read);
        return len_read;
    }

    return 0;
}

static int endpoint_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                 void *cb_arg) {
    LOG_DBG("Setting endpoint value %s", name);

    if (settings_name_steq(name, SETTING_PREFERRED_TRANSPORT_KEY, NULL)) {
        return endpoint_settings_load_preferred_v2(len, read_cb, cb_arg);
    }

    if (settings_name_steq(name, SETTING_PREFERRED_TRANSPORT_V1_KEY, NULL)) {
        return endpoint_settings_load_preferred_v1(len, read_cb, cb_arg);
    }

    return 0;
}

static int endpoint_settings_commit(void) {
    update_current_endpoint();
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(endpoints, SETTING_SUBTREE, NULL, endpoint_settings_set,
                               endpoint_settings_commit, NULL);

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
    switch (preferred_transport) {
    case ZMK_TRANSPORT_NONE:
        LOG_DBG("No endpoint transport selected");
        return ZMK_TRANSPORT_NONE;

    case ZMK_TRANSPORT_USB:
        if (is_usb_ready()) {
            LOG_DBG("USB is preferred and ready");
            return ZMK_TRANSPORT_USB;
        }
        if (is_ble_ready()) {
            LOG_DBG("USB is not ready. Falling back to BLE");
            return ZMK_TRANSPORT_BLE;
        }
        break;

    case ZMK_TRANSPORT_BLE:
        if (is_ble_ready()) {
            LOG_DBG("BLE is preferred and ready");
            return ZMK_TRANSPORT_BLE;
        }
        if (is_usb_ready()) {
            LOG_DBG("BLE is not ready. Falling back to USB");
            return ZMK_TRANSPORT_USB;
        }
        break;
    }

    LOG_DBG("Preferred endpoint transport is %d but no transports are ready", preferred_transport);
    return ZMK_TRANSPORT_NONE;
}

static struct zmk_endpoint_instance get_selected_instance(void) {
    return get_instance_from_transport(get_selected_transport());
}

static int zmk_endpoints_init(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&endpoints_save_work, endpoints_save_preferred_work);
#endif

    current_instance = get_selected_instance();

    return 0;
}

void zmk_endpoint_clear_reports(void) {
    zmk_hid_keyboard_clear();
    zmk_hid_consumer_clear();
#if IS_ENABLED(CONFIG_ZMK_POINTING)
    zmk_hid_mouse_clear();
#endif // IS_ENABLED(CONFIG_ZMK_POINTING)

    zmk_endpoint_send_report(HID_USAGE_KEY);
    zmk_endpoint_send_report(HID_USAGE_CONSUMER);
}

static void update_current_endpoint(void) {
    struct zmk_endpoint_instance new_instance = get_selected_instance();

    if (!zmk_endpoint_instance_eq(new_instance, current_instance)) {
        // Cancel all current keypresses so keys don't stay held on the old endpoint.
        zmk_endpoint_clear_reports();

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
