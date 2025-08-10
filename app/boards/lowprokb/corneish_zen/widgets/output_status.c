/*
 *
 * Copyright (c) 2021 Darryl deHaan
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include "output_status.h"
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

LV_IMG_DECLARE(bluetooth_advertising);
LV_IMG_DECLARE(bluetooth_connected_right);
LV_IMG_DECLARE(bluetooth_disconnected_right);
LV_IMG_DECLARE(bluetooth_connected_1);
LV_IMG_DECLARE(bluetooth_connected_2);
LV_IMG_DECLARE(bluetooth_connected_3);
LV_IMG_DECLARE(bluetooth_connected_4);
LV_IMG_DECLARE(bluetooth_connected_5);
LV_IMG_DECLARE(bluetooth_advertising_1);
LV_IMG_DECLARE(bluetooth_advertising_2);
LV_IMG_DECLARE(bluetooth_advertising_3);
LV_IMG_DECLARE(bluetooth_advertising_4);
LV_IMG_DECLARE(bluetooth_advertising_5);
LV_IMG_DECLARE(USB_connected);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
};

static struct output_status_state get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

static void set_status_symbol(lv_obj_t *icon, struct output_status_state state) {
    switch (state.selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        lv_img_set_src(icon, &USB_connected);
        break;
    case ZMK_TRANSPORT_BLE:
        if (state.active_profile_bonded) {
            if (state.active_profile_connected) {
                // sprintf(text, LV_SYMBOL_BLUETOOTH "%i " LV_SYMBOL_OK, active_profile_index);
                switch (state.selected_endpoint.ble.profile_index) {
                case 0:
                    lv_img_set_src(icon, &bluetooth_connected_1);
                    break;
                case 1:
                    lv_img_set_src(icon, &bluetooth_connected_2);
                    break;
                case 2:
                    lv_img_set_src(icon, &bluetooth_connected_3);
                    break;
                case 3:
                    lv_img_set_src(icon, &bluetooth_connected_4);
                    break;
                case 4:
                    lv_img_set_src(icon, &bluetooth_connected_5);
                    break;
                }
            } else {
                lv_img_set_src(icon, &bluetooth_disconnected_right);
            }
        } else {
            switch (state.selected_endpoint.ble.profile_index) {
            case 0:
                lv_img_set_src(icon, &bluetooth_advertising_1);
                break;
            case 1:
                lv_img_set_src(icon, &bluetooth_advertising_2);
                break;
            case 2:
                lv_img_set_src(icon, &bluetooth_advertising_3);
                break;
            case 3:
                lv_img_set_src(icon, &bluetooth_advertising_4);
                break;
            case 4:
                lv_img_set_src(icon, &bluetooth_advertising_5);
                break;
            }
        }
        break;
    }
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
// We don't get an endpoint changed event when the active profile connects/disconnects
// but there wasn't another endpoint to switch from/to, so update on BLE events too.
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent);

    sys_slist_append(&widgets, &widget->node);

    widget_output_status_init();
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}
