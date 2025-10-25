/*
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
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

struct peripheral_status_state {
    bool connected;
};

struct wpm_status_state {
    uint8_t wpm;
};

/* ==========================================================
 * Vẽ phần TOP (pin + kết nối)
 * ========================================================== */
static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_dsc;
    init_rect_dsc(&rect_dsc, LVGL_BACKGROUND);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_dsc);

    // Draw battery
    draw_battery(canvas, state);

    // Draw connection status (Wi-Fi symbol)
    lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc,
                        state->connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);

    rotate_canvas(canvas, cbuf);
}

/* ==========================================================
 * Vẽ phần WPM: biểu đồ + số
 * ========================================================== */
static void draw_wpm(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    if (!canvas) {
        LOG_ERR("WPM canvas not found!");
        return;
    }

    // Nền trắng, viền đen
    lv_draw_rect_dsc_t rect_white_dsc;
    lv_draw_rect_dsc_init(&rect_white_dsc);
    rect_white_dsc.bg_color = lv_color_white();
    rect_white_dsc.bg_opa = LV_OPA_COVER;
    rect_white_dsc.border_color = lv_color_black();
    rect_white_dsc.border_width = 1;
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_white_dsc);

    // Vẽ biểu đồ dạng cột
    lv_draw_rect_dsc_t bar_dsc;
    lv_draw_rect_dsc_init(&bar_dsc);
    bar_dsc.bg_color = lv_color_black();
    bar_dsc.bg_opa = LV_OPA_COVER;

    int bar_width = 4;
    int base_y = CANVAS_SIZE - 2;

    for (int i = 0; i < 10; i++) {
        int wpm_val = CLAMP(state->wpm[i], 0, 120);  // Giới hạn max WPM = 120
        int bar_height = (wpm_val * (CANVAS_SIZE - 4)) / 120;
        int x = 2 + i * (bar_width + 2);
        int y = base_y - bar_height;
        lv_canvas_draw_rect(canvas, x, y, bar_width, bar_height, &bar_dsc);
    }

    // Vẽ chữ "WPM" và giá trị hiện tại
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_unscii_8, LV_TEXT_ALIGN_LEFT);

    char buf[16];
    snprintf(buf, sizeof(buf), "WPM:%3d", state->wpm[9]);
    lv_canvas_draw_text(canvas, 5, 5, CANVAS_SIZE, &label_dsc, buf);

    rotate_canvas(canvas, cbuf);
}

/* ==========================================================
 * Battery + Connection
 * ========================================================== */
static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif
    widget->state.battery = state.level;
    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
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

/* ==========================================================
 * Peripheral connection (Wi-Fi icon)
 * ========================================================== */
static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void set_connection_status(struct zmk_widget_status *widget,
                                  struct peripheral_status_state state) {
    widget->state.connected = state.connected;
    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

/* ==========================================================
 * WPM: cập nhật từ central
 * ========================================================== */
static void set_wpm_status(struct zmk_widget_status *widget, struct wpm_status_state state) {
    for (int i = 0; i < 9; i++) {
        widget->state.wpm[i] = widget->state.wpm[i + 1];
    }
    widget->state.wpm[9] = state.wpm;

    draw_wpm(widget->obj, widget->cbuf2, &widget->state);
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(widget, state); }
}

static struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    const struct zmk_split_wpm_state_changed *ev = as_zmk_split_wpm_state_changed(eh);
    return (struct wpm_status_state){.wpm = (ev != NULL) ? ev->wpm : 0};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_split_wpm_state_changed);

/* ==========================================================
 * INIT
 * ========================================================== */
int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    // WPM canvas (trái)
    lv_obj_t *wpm_canvas = lv_canvas_create(widget->obj);
    lv_obj_align(wpm_canvas, LV_ALIGN_TOP_LEFT, -48, 0);
    lv_canvas_set_buffer(wpm_canvas, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    // Top canvas (phải)
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

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

    // Vẽ ban đầu
    draw_wpm(widget->obj, widget->cbuf2, &widget->state);
    draw_top(widget->obj, widget->cbuf, &widget->state);

    LOG_INF("Peripheral WPM widget initialized");

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
