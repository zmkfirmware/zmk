/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <bluetooth/services/bas.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display/widgets/battery_status.h>
#include <zmk/usb.h>
#include <zmk/events/usb-conn-state-changed.h>
#include <zmk/event-manager.h>
#include <zmk/events/battery-state-changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static lv_style_t label_style;

void battery_status_init() {
    if (label_style.text.font != NULL) {
        return;
    }

    lv_style_copy(&label_style, &lv_style_plain);
    label_style.text.color = LV_COLOR_BLACK;
    label_style.text.font = &lv_font_roboto_16;
    label_style.text.letter_space = 1;
    label_style.text.line_space = 1;
}

void set_battery_symbol(lv_obj_t *label) {
    char text[2] = "  ";
    u8_t level = bt_gatt_bas_get_battery_level();

#if IS_ENABLED(CONFIG_USB)
    if (zmk_usb_is_powered()) {
        strcpy(text, LV_SYMBOL_CHARGE);
    }
#endif /* IS_ENABLED(CONFIG_USB) */

    if (level > 95) {
        strcat(text, LV_SYMBOL_BATTERY_FULL);
    } else if (level > 65) {
        strcat(text, LV_SYMBOL_BATTERY_3);
    } else if (level > 35) {
        strcat(text, LV_SYMBOL_BATTERY_2);
    } else if (level > 5) {
        strcat(text, LV_SYMBOL_BATTERY_1);
    } else {
        strcat(text, LV_SYMBOL_BATTERY_EMPTY);
    }
    lv_label_set_text(label, text);
}

int zmk_widget_battery_status_init(struct zmk_widget_battery_status *widget, lv_obj_t *parent) {
    battery_status_init();
    widget->obj = lv_label_create(parent, NULL);
    lv_label_set_style(widget->obj, LV_LABEL_STYLE_MAIN, &label_style);

    lv_obj_set_size(widget->obj, 40, 15);
    set_battery_symbol(widget->obj);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_battery_status_obj(struct zmk_widget_battery_status *widget) {
    LOG_DBG("Label: %p", widget->obj);
    return widget->obj;
}

int battery_status_listener(const struct zmk_event_header *eh) {
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol(widget->obj); }
    return 0;
}

ZMK_LISTENER(widget_battery_status, battery_status_listener)
ZMK_SUBSCRIPTION(widget_battery_status, battery_state_changed);
#if IS_ENABLED(CONFIG_USB)
ZMK_SUBSCRIPTION(widget_battery_status, usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB) */
