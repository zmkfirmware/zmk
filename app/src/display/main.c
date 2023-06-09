/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "theme.h"

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/display/status_screen.h>

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
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

void display_timer_cb() { k_work_submit_to_queue(zmk_display_work_q(), &display_tick_work); }

K_TIMER_DEFINE(display_timer, display_timer_cb, NULL);

void unblank_display_cb(struct k_work *work) {
    display_blanking_off(display);
    k_timer_start(&display_timer, K_MSEC(TICK_MS), K_MSEC(TICK_MS));
}

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE)

void blank_display_cb(struct k_work *work) {
    k_timer_stop(&display_timer);
    display_blanking_on(display);
}
K_WORK_DEFINE(blank_display_work, blank_display_cb);
K_WORK_DEFINE(unblank_display_work, unblank_display_cb);

static void start_display_updates() {
    if (display == NULL) {
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &unblank_display_work);
}

static void stop_display_updates() {
    if (display == NULL) {
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &blank_display_work);
}

#endif

int zmk_display_is_initialized() { return initialized; }

static void initialize_theme() {
#if IS_ENABLED(CONFIG_LV_USE_THEME_MONO)
    lv_disp_t *disp = lv_disp_get_default();
    lv_theme_t *theme =
        lv_theme_mono_init(disp, IS_ENABLED(CONFIG_ZMK_DISPLAY_INVERT), CONFIG_LV_FONT_DEFAULT);
    theme->font_small = CONFIG_ZMK_LV_FONT_DEFAULT_SMALL;

    disp->theme = theme;
#endif // CONFIG_LV_USE_THEME_MONO
}

void initialize_display(struct k_work *work) {
    LOG_DBG("");

    if (!device_is_ready(display)) {
        LOG_ERR("Failed to find display device");
        return;
    }

    initialized = true;

    initialize_theme();

    screen = zmk_display_status_screen();

    if (screen == NULL) {
        LOG_ERR("No status screen provided");
        return;
    }

    lv_scr_load(screen);

    unblank_display_cb(work);
}

K_WORK_DEFINE(init_work, initialize_display);

int zmk_display_init() {
#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)
    k_work_queue_start(&display_work_q, display_work_stack_area,
                       K_THREAD_STACK_SIZEOF(display_work_stack_area),
                       CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_PRIORITY, NULL);
#endif

    k_work_submit_to_queue(zmk_display_work_q(), &init_work);

    LOG_DBG("");
    return 0;
}

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE)
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

#endif /* IS_ENABLED(CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE) */
