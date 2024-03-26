/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/midi_keys.h>

// in hid.c
#define ZMK_MIDI_NUM_KEYS 0x100

// should come after the last ZMK_HID_REPORT_ID in hid.h
#define ZMK_REPORT_ID_MIDI 0x04

#define ZMK_MIDI_CIN_NOTE_ON 0x90
#define ZMK_MIDI_CIN_NOTE_OFF 0x80
#define ZMK_MIDI_CIN_CONTROL_CHANGE 0xB0
#define ZMK_MIDI_CIN_PITCH_BEND_CHANGE 0xE0

#define ZMK_MIDI_MAX_VELOCITY 0x7F
#define ZMK_MIDI_ON_VELOCITY 0x3F
#define ZMK_MIDI_OFF_VELOCITY 0x64

#define ZMK_MIDI_TOGGLE_ON 0x7F
#define ZMK_MIDI_TOGGLE_OFF 0x0

// Analogous to zmk_hid_mouse_report_body in hid.h
struct zmk_midi_key_report_body {
    zmk_midi_cin_t cin;
    zmk_midi_key_t key;
    zmk_midi_value_t key_value;
} __packed;

// Analogous to zmk_hid_mouse_report in hid.h
struct zmk_midi_report {
    uint8_t report_id;
    struct zmk_midi_key_report_body body;
} __packed;

// Analogous to zmk_hid_mouse* in hid.h
int zmk_midi_key_press(zmk_midi_key_t key);
int zmk_midi_key_release(zmk_midi_key_t key);
void zmk_midi_clear(void);

// Analogous to zmk_hid_get_mouse_report in hid.h
struct zmk_midi_report *zmk_get_midi_report();
