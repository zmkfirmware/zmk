/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/preferred_transport_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

static const char TRANSPORT_SYMBOL_NONE[] = LV_SYMBOL_CLOSE;
static const char TRANSPORT_SYMBOL_BLE[] = LV_SYMBOL_WIFI;
static const char TRANSPORT_SYMBOL_USB[] = LV_SYMBOL_USB;
static const char TRANSPORT_SYMBOL_UNKNOWN[] = "?";

static const char ACTIVE_PROFILE_CONNECTED[] = LV_SYMBOL_OK;
static const char ACTIVE_PROFILE_DISCONNECTED[] = LV_SYMBOL_CLOSE;
static const char ACTIVE_PROFILE_UNBONDED[] = LV_SYMBOL_SETTINGS;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    enum zmk_transport preferred_transport;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};

static struct output_status_state get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoint_get_selected(),
        .preferred_transport = zmk_endpoint_get_preferred_transport(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

static const char *symbol_for_transport(enum zmk_transport transport) {
    switch (transport) {
    case ZMK_TRANSPORT_NONE:
        return TRANSPORT_SYMBOL_NONE;
    case ZMK_TRANSPORT_BLE:
        return TRANSPORT_SYMBOL_BLE;
    case ZMK_TRANSPORT_USB:
        return TRANSPORT_SYMBOL_USB;
    default:
        return TRANSPORT_SYMBOL_UNKNOWN;
    }
}

static const char *symbol_for_active_profile_status(const struct output_status_state state) {
    if (state.active_profile_bonded) {
        if (state.active_profile_connected) {
            return ACTIVE_PROFILE_CONNECTED;
        } else {
            return ACTIVE_PROFILE_DISCONNECTED;
        }
    } else {
        return ACTIVE_PROFILE_UNBONDED;
    }
}

static void set_status_symbol(lv_obj_t *label, struct output_status_state state) {
    char text[20] = {};

    enum zmk_transport transport = state.selected_endpoint.transport;
    const char *transport_symbol = symbol_for_transport(transport);
    bool disconnected = transport == ZMK_TRANSPORT_NONE;

    // If we are ble is connected or the preferred_transport show the active profile status
    // This allows you to navigate through the active ble profiles, even when they are unable
    // to be connected to.
    if (transport == ZMK_TRANSPORT_BLE || state.preferred_transport == ZMK_TRANSPORT_BLE) {
        // When the active transport is disconnected always use the ble symbol
        // Otherwise display the active transport.
        if (disconnected) {
            transport_symbol = TRANSPORT_SYMBOL_BLE;
        }

        const char *active_profile_status = symbol_for_active_profile_status(state);

        snprintf(text, sizeof(text), "%s %i %s", transport_symbol, state.active_profile_index + 1,
                 active_profile_status);

    } else if (disconnected && state.preferred_transport != ZMK_TRANSPORT_NONE) {
        // Indicate that we are disconnected from a preferred transport
        snprintf(text, sizeof(text), "%s " LV_SYMBOL_CLOSE,
                 symbol_for_transport(state.preferred_transport));
    } else {
        strcat(text, transport_symbol);
    }

    lv_label_set_text(label, text);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
// Update when the preferred transport changes but doesn't trigger an endpoint change
// eg. Connected to usb with ble preferred to preferred being usb.
ZMK_SUBSCRIPTION(widget_output_status, zmk_preferred_transport_changed);
// We don't get an endpoint changed event when the active profile connects/disconnects
// but there wasn't another endpoint to switch from/to, so update on BLE events too.
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    sys_slist_append(&widgets, &widget->node);

    widget_output_status_init();
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}
