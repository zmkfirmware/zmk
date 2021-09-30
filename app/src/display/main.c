/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
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
static bool initialized = false;

static lv_obj_t *screen;

__attribute__((weak)) lv_obj_t *zmk_display_status_screen() { return NULL; }

void display_tick_cb(struct k_work *work) { lv_task_handler(); }

#define TICK_MS 10

K_WORK_DEFINE(display_tick_work, display_tick_cb);

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)

K_THREAD_STACK_DEFINE(display_work_stack_area, CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE);

static struct k_work_q display_work_q;

#endif

struct k_work_q *zmk_display_work_q() {
#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)
    return &display_work_q;
#else
    return &k_sys_work_q;
#endif
}

void display_timer_cb() {
    lv_tick_inc(TICK_MS);
    k_work_submit_to_queue(zmk_display_work_q(), &display_tick_work);
}

void blank_display_cb(struct k_work *work) { display_blanking_on(display); }

void unblank_display_cb(struct k_work *work) { display_blanking_off(display); }

K_TIMER_DEFINE(display_timer, display_timer_cb, NULL);
K_WORK_DEFINE(blank_display_work, blank_display_cb);
K_WORK_DEFINE(unblank_display_work, unblank_display_cb);

static void start_display_updates() {
    if (display == NULL) {
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &unblank_display_work);

    k_timer_start(&display_timer, K_MSEC(TICK_MS), K_MSEC(TICK_MS));
}

static void stop_display_updates() {
    if (display == NULL) {
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &blank_display_work);

    k_timer_stop(&display_timer);
}

int zmk_display_is_initialized() { return initialized; }

int zmk_display_init() {
    LOG_DBG("");

    display = device_get_binding(ZMK_DISPLAY_NAME);
    if (display == NULL) {
        LOG_ERR("Failed to find display device");
        return -EINVAL;
    }

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)
    k_work_q_start(&display_work_q, display_work_stack_area,
                   K_THREAD_STACK_SIZEOF(display_work_stack_area),
                   CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_PRIORITY);
#endif

    screen = zmk_display_status_screen();

    if (screen == NULL) {
        LOG_ERR("No status screen provided");
        return 0;
    }

    lv_scr_load(screen);

    start_display_updates();

    initialized = true;

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
