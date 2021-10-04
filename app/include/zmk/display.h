/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/** @file display.h
 *  @brief Display functions and macros.
 */

#pragma once

struct k_work_q *zmk_display_work_q();

bool zmk_display_is_initialized();
int zmk_display_init();

/**
 * @brief Macro to define a ZMK event listener that handles the thread safety of fetching
 * the necessary state from the system work queue context, invoking a work callback
 * in the display queue context, and properly accessing that state safely when performing
 * display/LVGL updates.
 *
 * @param listener THe ZMK Event manager listener name.
 * @param state_type The struct/enum type used to store/transfer state.
 * @param cb The callback to invoke in the dispaly queue context to update the UI. Should be `void
 * func(state_type)` signature.
 * @param state_func The callback function to invoke to fetch the updated state from ZMK core.
 * Should be `state type func(const zmk_event_t *eh)` signature.
 * @retval listner##_init Generates a function `listener##_init` that should be called by the widget
 * once ready to be updated.
 **/
#define ZMK_DISPLAY_WIDGET_LISTENER(listener, state_type, cb, state_func)                          \
    K_MUTEX_DEFINE(listener##_mutex);                                                              \
    static state_type __##listener##_state;                                                        \
    static state_type listener##_get_local_state() {                                               \
        k_mutex_lock(&listener##_mutex, K_FOREVER);                                                \
        state_type copy = __##listener##_state;                                                    \
        k_mutex_unlock(&listener##_mutex);                                                         \
        return copy;                                                                               \
    };                                                                                             \
    static void listener##_work_cb(struct k_work *work) { cb(listener##_get_local_state()); };     \
    K_WORK_DEFINE(listener##_work, listener##_work_cb);                                            \
    static void listener##_refresh_state(const zmk_event_t *eh) {                                  \
        k_mutex_lock(&listener##_mutex, K_FOREVER);                                                \
        __##listener##_state = state_func(eh);                                                     \
        k_mutex_unlock(&listener##_mutex);                                                         \
    };                                                                                             \
    static void listener##_init() {                                                                \
        listener##_refresh_state(NULL);                                                            \
        listener##_work_cb(NULL);                                                                  \
    }                                                                                              \
    static int listener##_cb(const zmk_event_t *eh) {                                              \
        if (zmk_display_is_initialized()) {                                                        \
            listener##_refresh_state(eh);                                                          \
            k_work_submit_to_queue(zmk_display_work_q(), &listener##_work);                        \
        }                                                                                          \
        return ZMK_EV_EVENT_BUBBLE;                                                                \
    }                                                                                              \
    ZMK_LISTENER(listener, listener##_cb);
