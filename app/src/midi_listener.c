/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/events/midi_key_state_changed.h>
#include <zmk/endpoints.h>
#include <zmk/midi.h>

static void listener_midi_key_pressed(const struct zmk_midi_key_state_changed *ev) {
    LOG_DBG("midi key: 0x%04X", ev->key);
    int ret = zmk_midi_key_press(ev->key);
    if (ret < 0) {
        LOG_DBG("listener_midi_key_pressed received error, ignoring");
        return;
    }
    zmk_endpoints_send_midi_report();
}

static void listener_midi_key_released(const struct zmk_midi_key_state_changed *ev) {
    LOG_DBG("midi key: 0x%04X", ev->key);
    int ret = zmk_midi_key_release(ev->key);
    if (ret < 0) {
        LOG_DBG("listener_midi_key_released received error, ignoring");
        return;
    }
    zmk_endpoints_send_midi_report();
}

int midi_listener(const zmk_event_t *eh) {
    const struct zmk_midi_key_state_changed *midi_key_ev = as_zmk_midi_key_state_changed(eh);
    if (midi_key_ev) {
        if (midi_key_ev->state) {
            listener_midi_key_pressed(midi_key_ev);
        } else {
            listener_midi_key_released(midi_key_ev);
        }
        return 0;
    }
    return 0;
}

ZMK_LISTENER(midi_listener, midi_listener);
ZMK_SUBSCRIPTION(midi_listener, zmk_midi_key_state_changed);