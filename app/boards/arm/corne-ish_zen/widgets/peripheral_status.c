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
#include "peripheral_status.h"
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>

LV_IMG_DECLARE(bluetooth_connected_right);
LV_IMG_DECLARE(bluetooth_disconnected_right);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static lv_style_t label_style;

static bool style_initialized = false;

struct peripheral_status_state {
    bool connected;
};

//K_MUTEX_DEFINE(output_status_mutex);

static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void set_status_symbol(lv_obj_t *icon, struct peripheral_status_state state) {

    if (state.connected) {
        LOG_WRN("peripheral connected");
        lv_img_set_src(icon, &bluetooth_connected_right);
    }else{
        LOG_WRN("peripheral disconnected");
        lv_img_set_src(icon, &bluetooth_disconnected_right);
    }

    LOG_DBG("halves connected? %s", state.connected ? "true" : "false");
}

static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_peripheral_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

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

/*
void output_status_update_cb(struct k_work *work) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj); }
} */

int zmk_widget_peripheral_status_init(struct zmk_widget_peripheral_status *widget,
                                      lv_obj_t *parent) {
    output_status_init();

    widget->obj = lv_img_create(parent, NULL);

    sys_slist_append(&widgets, &widget->node);

    widget_peripheral_status_init();
    return 0;
}


lv_obj_t *zmk_widget_peripheral_status_obj(struct zmk_widget_peripheral_status *widget) {
    return widget->obj;
}






/*

static void update_state() {
    k_mutex_lock(&output_status_mutex, K_FOREVER);
    output_status_state.selected_endpoint = zmk_endpoints_selected();
    output_status_state.active_profile_connected = zmk_ble_active_profile_is_connected();
    output_status_state.active_profile_bonded = !zmk_ble_active_profile_is_open();
    output_status_state.active_profile_index = zmk_ble_active_profile_index();
    k_mutex_unlock(&output_status_mutex);
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
*/