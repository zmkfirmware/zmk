/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

enum usb_midi_error_t {
    USB_MIDI_SUCCESS = 0,
    USB_MIDI_ERROR_INVALID_CIN = -1,
    USB_MIDI_ERROR_INVALID_CABLE_NUM = -2,
    USB_MIDI_ERROR_INVALID_MIDI_MSG = -3
};

/* Code Index Numbers. See table 4-1 in the spec. */
enum usb_midi_cin_t {
    /* Miscellaneous function codes. Reserved for future extensions. */
    USB_MIDI_CIN_MISC = 0x0,
    /* Cable events. Reserved for future expansion. */
    USB_MIDI_CIN_CABLE_EVENT = 0x1,
    /* Two-byte System Common messages like MTC, SongSelect, etc. */
    USB_MIDI_CIN_SYSCOM_2BYTE = 0x2,
    /* Three-byte System Common messages like SPP, etc. */
    USB_MIDI_CIN_SYSCOM_3BYTE = 0x3,
    /* SysEx starts or continues */
    USB_MIDI_CIN_SYSEX_START_OR_CONTINUE = 0x4,
    /* Single-byte System Common Message or SysEx ends with following single byte. */
    USB_MIDI_CIN_SYS_COMMON_OR_SYSEX_END_1BYTE = 0x5,
    /* SysEx ends with following two bytes. */
    USB_MIDI_CIN_SYSEX_END_2BYTE = 0x6,
    /* SysEx ends with following three bytes. */
    USB_MIDI_CIN_SYSEX_END_3BYTE = 0x7,
    /* Note-off */
    USB_MIDI_CIN_NOTE_ON = 0x8,
    /* Note-on */
    USB_MIDI_CIN_NOTE_OFF = 0x9,
    /* Poly-KeyPress */
    USB_MIDI_CIN_POLY_KEYPRESS = 0xA,
    /* Control Change */
    USB_MIDI_CIN_CONTROL_CHANGE = 0xB,
    /* Program Change */
    USB_MIDI_CIN_PROGRAM_CHANGE = 0xC,
    /* Channel Pressure */
    USB_MIDI_CIN_CHANNEL_PRESSURE = 0xD,
    /* PitchBend Change */
    USB_MIDI_CIN_PITCH_BEND_CHANGE = 0xE,
    /* Single Byte */
    USB_MIDI_CIN_1BYTE_DATA = 0xF
};

/** Called when a non-sysex message has been parsed */
typedef void (*usb_midi_message_cb_t)(uint8_t *bytes, uint8_t num_bytes, uint8_t cable_num);
/** Called when a sysex message starts */
typedef void (*usb_midi_sysex_start_cb_t)(uint8_t cable_num);
/** Called when sysex data bytes have been received */
typedef void (*usb_midi_sysex_data_cb_t)(uint8_t *data_bytes, uint8_t num_data_bytes,
                                         uint8_t cable_num);
/** Called when a sysex message ends */
typedef void (*usb_midi_sysex_end_cb_t)(uint8_t cable_num);

struct usb_midi_parse_cb_t {
    usb_midi_message_cb_t message_cb;
    usb_midi_sysex_start_cb_t sysex_start_cb;
    usb_midi_sysex_data_cb_t sysex_data_cb;
    usb_midi_sysex_end_cb_t sysex_end_cb;
};

/* A USB MIDI event packet. See chapter 4 in the spec. */
struct usb_midi_packet_t {
    uint8_t cable_num;
    uint8_t cin;
    uint8_t bytes[4];
    uint8_t num_midi_bytes;
};

enum usb_midi_error_t usb_midi_packet_from_midi_bytes(uint8_t *midi_bytes, uint8_t cable_num,
                                                      struct usb_midi_packet_t *packet);
