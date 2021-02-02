/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <bluetooth/services/bas.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display/widgets/output_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static lv_style_t label_style;

static bool style_initialized = false;

void output_status_init() {
    if (style_initialized) {
        return;
    }

    style_initialized = true;
    lv_style_init(&label_style);
    lv_style_set_text_color(&label_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_font(&label_style, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_style_set_text_letter_space(&label_style, LV_STATE_DEFAULT, 1);
    lv_style_set_text_line_space(&label_style, LV_STATE_DEFAULT, 1);
}

void set_status_symbol(lv_obj_t *label) {
    enum zmk_endpoint selected_endpoint = zmk_endpoints_selected();
    bool active_profile_connected = zmk_ble_active_profile_is_connected();
    bool active_profie_bonded = !zmk_ble_active_profile_is_open();
    uint8_t active_profile_index = zmk_ble_active_profile_index();
    char text[6] = {};

    switch (selected_endpoint) {
    case ZMK_ENDPOINT_USB:
        strcat(text, LV_SYMBOL_USB "   ");
        break;
    case ZMK_ENDPOINT_BLE:
        if (active_profie_bonded) {
            if (active_profile_connected) {
                sprintf(text, LV_SYMBOL_WIFI "%i " LV_SYMBOL_OK, active_profile_index);
            } else {
                sprintf(text, LV_SYMBOL_WIFI "%i " LV_SYMBOL_CLOSE, active_profile_index);
            }
        } else {
            sprintf(text, LV_SYMBOL_WIFI "%i " LV_SYMBOL_SETTINGS, active_profile_index);
        }
        break;
    }

    lv_label_set_text(label, text);
}

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
    output_status_init();
    widget->obj = lv_label_create(parent, NULL);
    lv_obj_add_style(widget->obj, LV_LABEL_PART_MAIN, &label_style);

    lv_obj_set_size(widget->obj, 40, 15);
    set_status_symbol(widget->obj);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}

int output_status_listener(const zmk_event_t *eh) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj); }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_output_status, output_status_listener)
#if defined(CONFIG_USB)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif
