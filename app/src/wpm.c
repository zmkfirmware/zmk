/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/keycode_state_changed.h>

#include <zmk/workqueue.h>
#include <zmk/wpm.h>

#define WPM_UPDATE_INTERVAL_SECONDS 1
#define WPM_RESET_INTERVAL_SECONDS 5

// See https://en.wikipedia.org/wiki/Words_per_minute
// "Since the length or duration of words is clearly variable, for the purpose of measurement of
// text entry, the definition of each "word" is often standardized to be five characters or
// keystrokes long in English"
#define CHARS_PER_WORD 5.0

static uint8_t wpm_state = -1;
static uint8_t last_wpm_state;
static uint8_t wpm_update_counter;
static uint32_t key_pressed_count;

int zmk_wpm_get_state(void) { return wpm_state; }

int wpm_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev) {
        // count only key up events
        if (!ev->state) {
            key_pressed_count++;
            LOG_DBG("key_pressed_count %d keycode %d", key_pressed_count, ev->keycode);
        }
    }
    return 0;
}

void wpm_work_handler(struct k_work *work) {
    wpm_update_counter++;
    wpm_state = (key_pressed_count / CHARS_PER_WORD) /
                (wpm_update_counter * WPM_UPDATE_INTERVAL_SECONDS / 60.0);

    if (last_wpm_state != wpm_state) {
        LOG_DBG("Raised WPM state changed %d wpm_update_counter %d", wpm_state, wpm_update_counter);

        raise_zmk_wpm_state_changed((struct zmk_wpm_state_changed){.state = wpm_state});

        last_wpm_state = wpm_state;
    }

    if (wpm_update_counter >= WPM_RESET_INTERVAL_SECONDS) {
        wpm_update_counter = 0;
        key_pressed_count = 0;
    }
}

K_WORK_DEFINE(wpm_work, wpm_work_handler);

void wpm_expiry_function(struct k_timer *_timer) {
    k_work_submit_to_queue(zmk_main_work_q(), &wpm_work);
}

K_TIMER_DEFINE(wpm_timer, wpm_expiry_function, NULL);

static int wpm_init(void) {
    wpm_state = 0;
    wpm_update_counter = 0;
    k_timer_start(&wpm_timer, K_SECONDS(WPM_UPDATE_INTERVAL_SECONDS),
                  K_SECONDS(WPM_UPDATE_INTERVAL_SECONDS));
    return 0;
}

ZMK_LISTENER(wpm, wpm_event_listener);
ZMK_SUBSCRIPTION(wpm, zmk_keycode_state_changed);

SYS_INIT(wpm_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
