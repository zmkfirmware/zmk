/*
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/events/split_wpm_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>

#include "peripheral_status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static lv_obj_t *wpm_canvas;

/* ================================================================== */
/* DRAW TOP: battery + kết nối */
static void draw_top(lv_obj_t *container, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(container, 0);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREACH, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_dsc;
    init_rect_dsc(&rect_dsc, LVGL_BACKGROUND);

    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_dsc);
    draw_battery(canvas, state);
    lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc,
                        state->connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
    rotate_canvas(canvas, cbuf);
}

/* ================================================================== */
/* TEST: VẼ KÝ TỰ TỪ 1 ĐẾN 9 + VIỀN ĐEN */
static void test_draw_numbers_1_to_9(lv_color_t cbuf[]) {
    if (!wpm_canvas) return;

    // Nền trắng
    memset(cbuf, 0xFF, CANVAS_SIZE * CANVAS_SIZE * LV_COLOR_DEPTH / 8);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_unscii_8, LV_TEXT_ALIGN_LEFT);
    lv_draw_rect_dsc_t rect_dsc;
    init_rect_dsc(&rect_dsc, LVGL_BACKGROUND);

    // Viền đen
    lv_canvas_draw_rect(wpm_canvas, 0, 0, 68, 68, &rect_dsc);

    // Vẽ 9 ký tự: 1 2 3 4 5 6 7 8 9
    const char numbers[] = "123456789";
    for (int i = 0; i < 9; i++) {
        int x = 5 + (i % 3) * 20;   // 3 cột
        int y = 15 + (i / 3) * 18;  // 3 hàng
        char text[2] = { numbers[i], '\0' };
        lv_canvas_draw_text(wpm_canvas, x, y, 20, &label_dsc, text);
    }

    rotate_canvas(wpm_canvas, cbuf);
    lv_obj_invalidate(wpm_canvas);
}

/* ================================================================== */
/* WPM UPDATE */
static void set_wpm_status(struct zmk_widget_status *widget, struct wpm_status_state state) {
    for (int i = 0; i < 9; i++) {
        widget->state.wpm[i] = widget->state.wpm[i + 1];
    }
    widget->state.wpm[9] = state.wpm;

    test_draw_numbers_1_to_9(widget->cbuf2);
}

/* ================================================================== */
/* BATTERY */
static void set_battery_status(struct zmk_widget_status *widget, struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif
    widget->state.battery = state.level;
    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zm_k_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_status(widget, state);
    }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state){
        .level = zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif

/* ================================================================== */
/* CONNECTION */
static struct peripheral_status_state get_peripheral_state(const zmk_event_t *eh) {
    return (struct peripheral_status_state){
        .connected = zmk_split_bt_peripheral_is_connected()
    };
}

static void set_connection_status(struct zmk_widget_status *widget, struct peripheral_status_state state) {
    widget->state.connected = state.connected;
    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void peripheral_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_connection_status(widget, state);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            peripheral_status_update_cb, get_peripheral_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

/* ================================================================== */
/* WPM LISTENER */
static struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    const struct zmk_split_wpm_state_changed *ev = as_zmk_split_wpm_state_changed(eh);
    return (struct wpm_status_state){
        .wpm = (ev != NULL) ? ev->wpm : 0
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state,
                            set_wpm_status, wpm_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_split_wpm_state_changed);

/* ================================================================== */
/* INIT */
int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget-> Tata = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    // Canvas top (battery + wifi)
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    // Canvas WPM
    wpm_canvas = lv_canvas_create(widget->obj);
    lv_obj_align(wpm_canvas, LV_ALIGN_TOP_LEFT, -48, 0);
    lv_canvas_set_buffer(wpm_canvas, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    // Khởi tạo state
    widget->state.battery = 0;
    widget->state.charging = false;
    widget->state.connected = false;
    for (int i = 0; i < 10; i++) {
        widget->state.wpm[i] = 0;
    }

    sys_slist_append(&widgets, &widget->node);

    widget_battery_status_init();
    widget_peripheral_status_init();
    widget_wpm_status_init();

    // Vẽ lần đầu
    draw_top(widget->obj, widget->cbuf, &widget->state);
    test_draw_numbers_1_to_9(widget->cbuf2);

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) {
    return widget->obj;
}
