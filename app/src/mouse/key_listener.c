/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/mouse_button_state_changed.h>
#include <zmk/events/mouse_move_state_changed.h>
#include <zmk/events/mouse_scroll_state_changed.h>
#include <zmk/events/mouse_tick.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/mouse.h>

static struct vector2d move_speed = {0};
static struct vector2d scroll_speed = {0};
static struct mouse_config move_config = {0};
static struct mouse_config scroll_config = {0};

static void clear_mouse_state() {
    move_speed = (struct vector2d){0};
    scroll_speed = (struct vector2d){0};
}

static void mouse_tick_timer_handler(struct k_work *work) {
    zmk_hid_mouse_movement_set(0, 0);
    zmk_hid_mouse_scroll_set(0, 0);
    ZMK_EVENT_RAISE(zmk_mouse_tick(move_speed, scroll_speed, move_config, scroll_config));
    zmk_endpoints_send_mouse_report();
}

K_WORK_DEFINE(mouse_tick, &mouse_tick_timer_handler);

void mouse_timer_cb(struct k_timer *dummy) {
    k_work_submit_to_queue(zmk_mouse_work_q(), &mouse_tick);
}

K_TIMER_DEFINE(mouse_timer, mouse_timer_cb, mouse_timer_cb);

static int mouse_timer_ref_count = 0;

void mouse_timer_ref() {
    if (mouse_timer_ref_count == 0) {
        k_timer_start(&mouse_timer, K_NO_WAIT, K_MSEC(10));
    }
    mouse_timer_ref_count += 1;
    // trigger the first mouse tick event immediately
    mouse_tick_timer_handler(NULL);
}

void mouse_timer_unref() {
    if (mouse_timer_ref_count > 0) {
        mouse_timer_ref_count--;
    }
    if (mouse_timer_ref_count == 0) {
        k_timer_stop(&mouse_timer);
        clear_mouse_state();
    }
}

static void listener_mouse_move_pressed(const struct zmk_mouse_move_state_changed *ev) {
    move_speed.x += ev->max_speed.x;
    move_speed.y += ev->max_speed.y;
    mouse_timer_ref();
}

static void listener_mouse_move_released(const struct zmk_mouse_move_state_changed *ev) {
    move_speed.x -= ev->max_speed.x;
    move_speed.y -= ev->max_speed.y;
    mouse_timer_unref();
}

static void listener_mouse_scroll_pressed(const struct zmk_mouse_scroll_state_changed *ev) {
    scroll_speed.x += ev->max_speed.x;
    scroll_speed.y += ev->max_speed.y;
    mouse_timer_ref();
}

static void listener_mouse_scroll_released(const struct zmk_mouse_scroll_state_changed *ev) {
    scroll_speed.x -= ev->max_speed.x;
    scroll_speed.y -= ev->max_speed.y;
    mouse_timer_unref();
}

static void listener_mouse_button_pressed(const struct zmk_mouse_button_state_changed *ev) {
    LOG_DBG("buttons: 0x%02X", ev->buttons);
    zmk_hid_mouse_buttons_press(ev->buttons);
    zmk_endpoints_send_mouse_report();
}

static void listener_mouse_button_released(const struct zmk_mouse_button_state_changed *ev) {
    LOG_DBG("buttons: 0x%02X", ev->buttons);
    zmk_hid_mouse_buttons_release(ev->buttons);
    zmk_endpoints_send_mouse_report();
}

int mouse_listener(const zmk_event_t *eh) {
    const struct zmk_mouse_move_state_changed *mmv_ev = as_zmk_mouse_move_state_changed(eh);
    if (mmv_ev) {
        if (mmv_ev->state) {
            listener_mouse_move_pressed(mmv_ev);
        } else {
            listener_mouse_move_released(mmv_ev);
        }
        return 0;
    }
    const struct zmk_mouse_scroll_state_changed *msc_ev = as_zmk_mouse_scroll_state_changed(eh);
    if (msc_ev) {
        if (msc_ev->state) {
            listener_mouse_scroll_pressed(msc_ev);
        } else {
            listener_mouse_scroll_released(msc_ev);
        }
        return 0;
    }
    const struct zmk_mouse_button_state_changed *mbt_ev = as_zmk_mouse_button_state_changed(eh);
    if (mbt_ev) {
        if (mbt_ev->state) {
            listener_mouse_button_pressed(mbt_ev);
        } else {
            listener_mouse_button_released(mbt_ev);
        }
        return 0;
    }
    return 0;
}

ZMK_LISTENER(mouse_listener, mouse_listener);
ZMK_SUBSCRIPTION(mouse_listener, zmk_mouse_button_state_changed);
ZMK_SUBSCRIPTION(mouse_listener, zmk_mouse_move_state_changed);
ZMK_SUBSCRIPTION(mouse_listener, zmk_mouse_scroll_state_changed);
