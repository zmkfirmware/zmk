/* SPDX-License-Identifier: MIT */
#include "calculator.h"
#include <stdio.h>
#include <dt-bindings/zmk/keys.h>
#include <zmk/endpoints.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

/* This worker only emits one press or release per invocation. */
K_MUTEX_DEFINE(sender_mutex);
static char pending[CALC_RESULT_SIZE];
static size_t cursor;
static uint32_t held;
static bool busy;
static atomic_t interrupted;
static struct zmk_endpoint_instance destination;

static uint32_t encode(char c) {
    static const uint32_t digits[] = {N0, N1, N2, N3, N4, N5, N6, N7, N8, N9};
    if (c >= '0' && c <= '9') {
        return digits[c - '0'];
    }
    switch (c) {
    case '-':
        return MINUS;
    case '.':
        return DOT;
    case 'e':
        return E;
    default:
        return 0;
    }
}

static bool modifiers_down(void) {
    for (int i = 0; i < 8; i++) {
        if (zmk_hid_mod_is_pressed(i)) {
            return true;
        }
    }
    return false;
}

static void send_work(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sender_work, send_work);

static void send_work(struct k_work *work) {
    const char *notice = NULL;
    k_mutex_lock(&sender_mutex, K_FOREVER);
    if (!busy) {
        k_mutex_unlock(&sender_mutex);
        return;
    }
    bool stop = atomic_get(&interrupted) || !zmk_endpoint_is_connected() ||
                !zmk_endpoint_instance_eq(destination, zmk_endpoint_get_selected());
    if (held) {
        /* Also release our local HID state after disconnect/endpoint changes. */
        raise_zmk_keycode_state_changed_from_encoded(held, false, k_uptime_get());
        held = 0;
    } else if (!stop && pending[cursor]) {
        uint32_t code = encode(pending[cursor]);
        if (!code || modifiers_down() || zmk_hid_is_pressed(code)) {
            stop = true;
        } else {
            held = code;
            cursor++;
            raise_zmk_keycode_state_changed_from_encoded(held, true, k_uptime_get());
        }
    }
    if (stop || (!held && !pending[cursor])) {
        busy = false;
        notice = stop ? "Send stopped" : "Sent";
    } else {
        k_work_reschedule(&sender_work, K_MSEC(20));
    }
    k_mutex_unlock(&sender_mutex);
    if (notice) {
        numpad_calculator_notify(notice);
    }
}

int numpad_calculator_send(const char *text) {
    const char *notice = "Sending";
    int ret = 0;
    k_mutex_lock(&sender_mutex, K_FOREVER);
    if (busy) {
        notice = "Sending";
        ret = -EBUSY;
    } else if (!zmk_endpoint_is_connected()) {
        notice = "Not connected";
        ret = -ENOTCONN;
    } else if (modifiers_down()) {
        notice = "Release modifiers";
        ret = -EBUSY;
    } else {
        snprintf(pending, sizeof(pending), "%s", text);
        cursor = 0;
        held = 0;
        atomic_clear(&interrupted);
        destination = zmk_endpoint_get_selected();
        busy = true;
        k_work_reschedule(&sender_work, K_MSEC(20));
    }
    k_mutex_unlock(&sender_mutex);
    numpad_calculator_notify(notice);
    return ret;
}

void numpad_calculator_send_cancel(void) {
    atomic_set(&interrupted, 1);
    /* Do not reschedule: a press must retain its minimum hold duration. */
}

static int endpoint_changed(const zmk_event_t *eh) {
    numpad_calculator_send_cancel();
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(numpad_calculator_sender, endpoint_changed);
ZMK_SUBSCRIPTION(numpad_calculator_sender, zmk_endpoint_changed);
