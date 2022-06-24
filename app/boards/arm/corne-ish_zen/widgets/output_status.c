/*
*
* Copyright (c) 2021 Darryl deHaan
* SPDX-License-Identifier: MIT
*
*/

#include <kernel.h>
#include <bluetooth/services/bas.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include "output_status.h"
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_selection_changed.h>
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
static lv_style_t label_style;

static bool style_initialized = false;

K_MUTEX_DEFINE(output_status_mutex);

struct {
    enum zmk_endpoint selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t active_profile_index;
} output_status_state;

void output_status_init() {
    if (style_initialized) {
        return;
    }

    style_initialized = true;
    lv_style_init(&label_style);
    lv_style_set_text_color(&label_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_font(&label_style, LV_STATE_DEFAULT, &lv_font_montserrat_26);
    lv_style_set_text_letter_space(&label_style, LV_STATE_DEFAULT, 1);
    lv_style_set_text_line_space(&label_style, LV_STATE_DEFAULT, 1);
}

void set_status_symbol(lv_obj_t *icon) {

   k_mutex_lock(&output_status_mutex, K_FOREVER);
    enum zmk_endpoint selected_endpoint = output_status_state.selected_endpoint;
    bool active_profile_connected = output_status_state.active_profile_connected;
    bool active_profie_bonded = output_status_state.active_profile_bonded;
    uint8_t active_profile_index = output_status_state.active_profile_index;
    k_mutex_unlock(&output_status_mutex);

    switch (selected_endpoint) {
    case ZMK_ENDPOINT_USB:
        lv_img_set_src(icon, &USB_connected);
        break;
    case ZMK_ENDPOINT_BLE:
        if (active_profie_bonded) {
            if (active_profile_connected) {
                //sprintf(text, LV_SYMBOL_BLUETOOTH "%i " LV_SYMBOL_OK, active_profile_index);
                switch (active_profile_index) {
                case 0:
#if CONFIG_BOARD_CORNEISH_ZEN_RIGHT
                    lv_img_set_src(icon, &bluetooth_connected_right);
#else
                    lv_img_set_src(icon, &bluetooth_connected_1);
#endif
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
                //sprintf(text, LV_SYMBOL_BLUETOOTH "%i " LV_SYMBOL_CLOSE, active_profile_index);
                lv_img_set_src(icon, &bluetooth_disconnected_right);
            }
        } else {
            //sprintf(text, LV_SYMBOL_BLUETOOTH "%i " LV_SYMBOL_SETTINGS, active_profile_index);
            //lv_img_set_src(icon, &bluetooth_advertising);
            switch (active_profile_index) {
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

    //lv_label_set_text(label, text);
}

static void update_state() {
    k_mutex_lock(&output_status_mutex, K_FOREVER);
    output_status_state.selected_endpoint = zmk_endpoints_selected();
    output_status_state.active_profile_connected = zmk_ble_active_profile_is_connected();
    output_status_state.active_profile_bonded = !zmk_ble_active_profile_is_open();
    output_status_state.active_profile_index = zmk_ble_active_profile_index();
    k_mutex_unlock(&output_status_mutex);
}

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
    output_status_init();
    //update_state();
    //widget->obj = lv_label_create(parent, NULL);
    widget->obj = lv_img_create(parent, NULL);
  
    set_status_symbol(widget->obj);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}

void output_status_update_cb(struct k_work *work) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj); }
}

K_WORK_DEFINE(output_status_update_work, output_status_update_cb);

int output_status_listener(const zmk_event_t *eh) {


    // Be sure we have widgets initialized before doing any work,
    // since the status event can fire before display code inits.
    if (!style_initialized) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    
    //update_state();

    k_work_submit_to_queue(zmk_display_work_q(), &output_status_update_work);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_output_status, output_status_listener)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_selection_changed);
#if defined(CONFIG_USB)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif