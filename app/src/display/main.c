/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <init.h>
#include <device.h>
#include <devicetree.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/display.h>
#include <lvgl.h>

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/display/status_screen.h>

#define ZMK_DISPLAY_NAME CONFIG_LVGL_DISPLAY_DEV_NAME

static const struct device *display;

static lv_obj_t *screen;

__attribute__((weak)) lv_obj_t *zmk_display_status_screen() { return NULL; }

void display_tick_cb(struct k_work *work) {
    lv_tick_inc(10);
    lv_task_handler();
}

K_WORK_DEFINE(display_tick_work, display_tick_cb);

void display_timer_cb() { k_work_submit(&display_tick_work); }

K_TIMER_DEFINE(display_timer, display_timer_cb, NULL);

static void start_display_updates() {
    if (display == NULL) {
        return;
    }

    display_blanking_off(display);

    k_timer_start(&display_timer, K_MSEC(10), K_MSEC(10));
}

static void stop_display_updates() {
    if (display == NULL) {
        return;
    }

    display_blanking_on(display);

    k_timer_stop(&display_timer);
}

int zmk_display_init() {
    LOG_DBG("");

    display = device_get_binding(ZMK_DISPLAY_NAME);
    if (display == NULL) {
        LOG_ERR("Failed to find display device");
        return -EINVAL;
    }

    screen = zmk_display_status_screen();

    if (screen == NULL) {
        LOG_ERR("No status screen provided");
        return 0;
    }

    lv_scr_load(screen);

    lv_task_handler();

    start_display_updates();

    LOG_DBG("");
    return 0;
}

int display_event_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    switch (ev->state) {
    case ZMK_ACTIVITY_ACTIVE:
        start_display_updates();
        break;
    case ZMK_ACTIVITY_IDLE:
    case ZMK_ACTIVITY_SLEEP:
        stop_display_updates();
        break;
    default:
        LOG_WRN("Unhandled activity state: %d", ev->state);
        return -EINVAL;
    }
    return 0;
}

ZMK_LISTENER(display, display_event_handler);
ZMK_SUBSCRIPTION(display, zmk_activity_state_changed);