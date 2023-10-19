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
#include "battery_status.h"
#include <zmk/usb.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_status_state {
    uint8_t level;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_present;
#endif
};

LV_IMG_DECLARE(batt_100);
LV_IMG_DECLARE(batt_100_chg);
LV_IMG_DECLARE(batt_75);
LV_IMG_DECLARE(batt_75_chg);
LV_IMG_DECLARE(batt_50);
LV_IMG_DECLARE(batt_50_chg);
LV_IMG_DECLARE(batt_25);
LV_IMG_DECLARE(batt_25_chg);
LV_IMG_DECLARE(batt_5);
LV_IMG_DECLARE(batt_5_chg);
LV_IMG_DECLARE(batt_0);
LV_IMG_DECLARE(batt_0_chg);

static void set_battery_symbol(lv_obj_t *icon, struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    static uint8_t stage_prev = 255;
    static bool usb_prev = false;

    uint8_t level = state.level;
    bool usb_present = state.usb_present;
    uint8_t stage;

    if (level > 87) {
        stage = 5;
    } else if (level > 62) {
        stage = 4;
    } else if (level > 37) {
        stage = 3;
    } else if (level > 12) {
        stage = 2;
    } else if (level > 5) {
        stage = 1;
    } else {
        stage = 0;
    }

    // check if there is a change requiring an update
    if (usb_present != usb_prev || stage != stage_prev) {
        switch (stage) {
        case 5:
            lv_img_set_src(icon, usb_present ? &batt_100_chg : &batt_100);
            break;
        case 4:
            lv_img_set_src(icon, usb_present ? &batt_75_chg : &batt_75);
            break;
        case 3:
            lv_img_set_src(icon, usb_present ? &batt_50_chg : &batt_50);
            break;
        case 2:
            lv_img_set_src(icon, usb_present ? &batt_25_chg : &batt_25);
            break;
        case 1:
            lv_img_set_src(icon, usb_present ? &batt_5_chg : &batt_5);
            break;
        default:
            lv_img_set_src(icon, usb_present ? &batt_0_chg : &batt_0);
            break;
        }
        usb_prev = usb_present;
        stage_prev = stage;
    }
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
}

void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol(widget->obj, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state) {
        .level = bt_bas_get_battery_level(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

int zmk_widget_battery_status_init(struct zmk_widget_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();

    return 0;
}

lv_obj_t *zmk_widget_battery_status_obj(struct zmk_widget_battery_status *widget) {
    return widget->obj;
}
