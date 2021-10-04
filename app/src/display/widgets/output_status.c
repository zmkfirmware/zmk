/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <bluetooth/services/bas.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_selection_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    enum zmk_endpoint selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t active_profile_index;
};

static struct output_status_state get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){.selected_endpoint = zmk_endpoints_selected(),
                                        .active_profile_connected =
                                            zmk_ble_active_profile_is_connected(),
                                        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
                                        .active_profile_index = zmk_ble_active_profile_index()};
    ;
}

static void set_status_symbol(lv_obj_t *label, struct output_status_state state) {
    char text[6] = {};

    switch (state.selected_endpoint) {
    case ZMK_ENDPOINT_USB:
        strcat(text, LV_SYMBOL_USB "   ");
        break;
    case ZMK_ENDPOINT_BLE:
        if (state.active_profile_bonded) {
            if (state.active_profile_connected) {
                snprintf(text, sizeof(text), LV_SYMBOL_WIFI "%i " LV_SYMBOL_OK,
                         state.active_profile_index);
            } else {
                snprintf(text, sizeof(text), LV_SYMBOL_WIFI "%i " LV_SYMBOL_CLOSE,
                         state.active_profile_index);
            }
        } else {
            snprintf(text, sizeof(text), LV_SYMBOL_WIFI "%i " LV_SYMBOL_SETTINGS,
                     state.active_profile_index);
        }
        break;
    }

    lv_label_set_text(label, text);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_selection_changed);

#if defined(CONFIG_USB)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent, NULL);

    lv_obj_set_size(widget->obj, 40, 15);

    sys_slist_append(&widgets, &widget->node);

    widget_output_status_init();
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}
