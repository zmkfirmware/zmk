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

#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>

#define HAS_PAIRING_SCREEN                                                                         \
    (IS_ENABLED(CONFIG_ZMK_DISPLAY_PAIRING_SCREEN_BUILT_IN) ||                                     \
     IS_ENABLED(CONFIG_ZMK_DISPLAY_PAIRING_SCREEN_CUSTOM))

#if HAS_PAIRING_SCREEN
#include <zmk/ble.h>
#include <zmk/display/pairing_screen.h>
#include <zmk/events/ble_auth_state_changed.h>
#endif

#define TICK_MS 10

enum screen_type {
    SCREEN_TYPE_STATUS,
#if HAS_PAIRING_SCREEN
    SCREEN_TYPE_PAIRING,
#endif
};

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static lv_obj_t *current_screen = NULL;
static bool initialized = false;

static lv_obj_t *status_screen = NULL;
#if HAS_PAIRING_SCREEN
static lv_obj_t *pairing_screen = NULL;
#endif

__attribute__((weak)) lv_obj_t *zmk_display_status_screen(void) { return NULL; }
#if HAS_PAIRING_SCREEN
__attribute__((weak)) lv_obj_t *zmk_display_pairing_screen(void) { return NULL; }
#endif

void display_tick_cb(struct k_work *work) { lv_task_handler(); }

K_WORK_DEFINE(display_tick_work, display_tick_cb);

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)

K_THREAD_STACK_DEFINE(display_work_stack_area, CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE);

static struct k_work_q display_work_q;

#endif

struct k_work_q *zmk_display_work_q(void) {
#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)
    return &display_work_q;
#else
    return &k_sys_work_q;
#endif
}

static void display_timer_cb(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_display_work_q(), &display_tick_work);
}

K_TIMER_DEFINE(display_timer, display_timer_cb, NULL);

static void unblank_display_cb(struct k_work *work) {
    display_blanking_off(display);
    k_timer_start(&display_timer, K_MSEC(TICK_MS), K_MSEC(TICK_MS));
}

static void blank_display_cb(struct k_work *work) {
    k_timer_stop(&display_timer);
    display_blanking_on(display);
}
K_WORK_DEFINE(blank_display_work, blank_display_cb);
K_WORK_DEFINE(unblank_display_work, unblank_display_cb);

void zmk_display_blanking_off(void) {
    if (display == NULL) {
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &unblank_display_work);
}

void zmk_display_blanking_on(void) {
    if (display == NULL) {
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &blank_display_work);
}

int zmk_display_is_initialized(void) { return initialized; }

K_MUTEX_DEFINE(screen_state_mutex);
#if HAS_PAIRING_SCREEN
static struct zmk_ble_auth_state ble_auth_state;
#endif

static enum screen_type get_screen_type(void) {
    enum screen_type screen_type = SCREEN_TYPE_STATUS;

#if HAS_PAIRING_SCREEN
    k_mutex_lock(&screen_state_mutex, K_FOREVER);

    if (ble_auth_state.mode != ZMK_BLE_AUTH_MODE_NONE) {
        screen_type = SCREEN_TYPE_PAIRING;
    }

    k_mutex_unlock(&screen_state_mutex);
#endif

    return screen_type;
}

static void update_screen(struct k_work *work) {
    enum screen_type new_screen_type = get_screen_type();
    lv_obj_t *new_screen = NULL;

    switch (new_screen_type) {
    case SCREEN_TYPE_STATUS:
        new_screen = status_screen;
        break;

#if HAS_PAIRING_SCREEN
    case SCREEN_TYPE_PAIRING:
        new_screen = pairing_screen;
        break;
#endif

    default:
        LOG_ERR("Unhandled screen type: %d", new_screen_type);
    }

    if (new_screen != current_screen) {
        current_screen = new_screen;
        lv_scr_load(current_screen);
        unblank_display_cb(work);
    }
}

K_WORK_DEFINE(update_screen_work, update_screen);

static void initialize_display(struct k_work *work) {
    update_screen(work);

    if (!current_screen) {
        LOG_ERR("No status screen provided");
    }

    initialized = true;
}

K_WORK_DEFINE(init_work, initialize_display);

int zmk_display_init(void) {
    if (!device_is_ready(display)) {
        LOG_ERR("Failed to find display device");
        return -ENODEV;
    }

    zmk_display_initialize_theme();

    status_screen = zmk_display_status_screen();
#if HAS_PAIRING_SCREEN
    pairing_screen = zmk_display_pairing_screen();
#endif

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)
    k_work_queue_start(&display_work_q, display_work_stack_area,
                       K_THREAD_STACK_SIZEOF(display_work_stack_area),
                       CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_PRIORITY, NULL);
#endif

    k_work_submit_to_queue(zmk_display_work_q(), &init_work);

    return 0;
}

#if HAS_PAIRING_SCREEN
static int handle_ble_auth_state_changed(const struct zmk_ble_auth_state_changed *ev) {
    k_mutex_lock(&screen_state_mutex, K_FOREVER);
    ble_auth_state = ev->state;
    k_mutex_unlock(&screen_state_mutex);

    k_work_submit_to_queue(zmk_display_work_q(), &update_screen_work);
    return ZMK_EV_EVENT_BUBBLE;
}

static int display_event_handler(const zmk_event_t *eh) {
    const struct zmk_ble_auth_state_changed *auth_ev = as_zmk_ble_auth_state_changed(eh);
    if (auth_ev) {
        return handle_ble_auth_state_changed(auth_ev);
    }

    return -ENOTSUP;
}

ZMK_LISTENER(display, display_event_handler);
ZMK_SUBSCRIPTION(display, zmk_ble_auth_state_changed);
#endif // HAS_PAIRING_SCREEN
