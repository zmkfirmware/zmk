/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <zmk/keys.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

#define COLLECTION_REPORT 0x03

#define ZMK_HID_KEYBOARD_NKRO_SIZE 6

#define ZMK_HID_CONSUMER_NKRO_SIZE 6

static const uint8_t zmk_hid_report_desc[] = {
    /* USAGE_PAGE (Generic Desktop) */
    HID_GI_USAGE_PAGE,
    HID_USAGE_GD,
    /* USAGE (Keyboard) */
    HID_LI_USAGE,
    HID_USAGE_GD_KEYBOARD,
    /* COLLECTION (Application) */
    HID_MI_COLLECTION,
    COLLECTION_APPLICATION,
    /* REPORT ID (1) */
    HID_GI_REPORT_ID,
    0x01,
    /* USAGE_PAGE (Keyboard/Keypad) */
    HID_GI_USAGE_PAGE,
    HID_USAGE_KEY,
    /* USAGE_MINIMUM (Keyboard LeftControl) */
    HID_LI_USAGE_MIN(1),
    HID_USAGE_KEY_KEYBOARD_LEFTCONTROL,
    /* USAGE_MAXIMUM (Keyboard Right GUI) */
    HID_LI_USAGE_MAX(1),
    HID_USAGE_KEY_KEYBOARD_RIGHT_GUI,
    /* LOGICAL_MINIMUM (0) */
    HID_GI_LOGICAL_MIN(1),
    0x00,
    /* LOGICAL_MAXIMUM (1) */
    HID_GI_LOGICAL_MAX(1),
    0x01,

    /* REPORT_SIZE (1) */
    HID_GI_REPORT_SIZE,
    0x01,
    /* REPORT_COUNT (8) */
    HID_GI_REPORT_COUNT,
    0x08,
    /* INPUT (Data,Var,Abs) */
    HID_MI_INPUT,
    0x02,

    /* USAGE_PAGE (Keyboard/Keypad) */
    HID_GI_USAGE_PAGE,
    HID_USAGE_KEY,
    /* REPORT_SIZE (8) */
    HID_GI_REPORT_SIZE,
    0x08,
    /* REPORT_COUNT (1) */
    HID_GI_REPORT_COUNT,
    0x01,
    /* INPUT (Cnst,Var,Abs) */
    HID_MI_INPUT,
    0x03,

    /* USAGE_PAGE (Keyboard/Keypad) */
    HID_GI_USAGE_PAGE,
    HID_USAGE_KEY,
    /* LOGICAL_MINIMUM (0) */
    HID_GI_LOGICAL_MIN(1),
    0x00,
    /* LOGICAL_MAXIMUM (0xFF) */
    HID_GI_LOGICAL_MAX(1),
    0xFF,
    /* USAGE_MINIMUM (Reserved) */
    HID_LI_USAGE_MIN(1),
    0x00,
    /* USAGE_MAXIMUM (Keyboard Application) */
    HID_LI_USAGE_MAX(1),
    0xFF,
    /* REPORT_SIZE (1) */
    HID_GI_REPORT_SIZE,
    0x08,
    /* REPORT_COUNT (ZMK_HID_KEYBOARD_NKRO_SIZE) */
    HID_GI_REPORT_COUNT,
    ZMK_HID_KEYBOARD_NKRO_SIZE,
    /* INPUT (Data,Ary,Abs) */
    HID_MI_INPUT,
    0x00,

    /* END_COLLECTION */
    HID_MI_COLLECTION_END,
    /* USAGE_PAGE (Consumer) */
    HID_GI_USAGE_PAGE,
    HID_USAGE_CONSUMER,
    /* USAGE (Consumer Control) */
    HID_LI_USAGE,
    HID_USAGE_CONSUMER_CONSUMER_CONTROL,
    /* Consumer Page */
    HID_MI_COLLECTION,
    COLLECTION_APPLICATION,
    /* REPORT ID (1) */
    HID_GI_REPORT_ID,
    0x02,
    /* USAGE_PAGE (Consumer) */
    HID_GI_USAGE_PAGE,
    HID_USAGE_CONSUMER,
    /* LOGICAL_MINIMUM (0) */
    HID_GI_LOGICAL_MIN(1),
    0x00,
    /* LOGICAL_MAXIMUM (0xFFFF) */
    HID_GI_LOGICAL_MAX(2),
    0xFF,
    0xFF,
    HID_LI_USAGE_MIN(1),
    0x00,
    /* USAGE_MAXIMUM (0xFFFF) */
    HID_LI_USAGE_MAX(2),
    0xFF,
    0xFF,
    /* INPUT (Data,Ary,Abs) */
    /* REPORT_SIZE (16) */
    HID_GI_REPORT_SIZE,
    0x10,
    /* REPORT_COUNT (ZMK_HID_CONSUMER_NKRO_SIZE) */
    HID_GI_REPORT_COUNT,
    ZMK_HID_CONSUMER_NKRO_SIZE,
    HID_MI_INPUT,
    0x00,
    /* END COLLECTION */
    HID_MI_COLLECTION_END,
};

// struct zmk_hid_boot_report
// {
//     uint8_t modifiers;
//     uint8_t _unused;
//     uint8_t keys[6];
// } __packed;

struct zmk_hid_keyboard_report_body {
    zmk_mod_flags_t modifiers;
    uint8_t _reserved;
    uint8_t keys[ZMK_HID_KEYBOARD_NKRO_SIZE];
} __packed;

struct zmk_hid_keyboard_report {
    uint8_t report_id;
    struct zmk_hid_keyboard_report_body body;
} __packed;

struct zmk_hid_consumer_report_body {
    uint16_t keys[ZMK_HID_CONSUMER_NKRO_SIZE];
} __packed;

struct zmk_hid_consumer_report {
    uint8_t report_id;
    struct zmk_hid_consumer_report_body body;
} __packed;

zmk_mod_flags_t zmk_hid_get_explicit_mods();
int zmk_hid_register_mod(zmk_mod_t modifier);
int zmk_hid_unregister_mod(zmk_mod_t modifier);
int zmk_hid_register_mods(zmk_mod_flags_t explicit_modifiers);
int zmk_hid_unregister_mods(zmk_mod_flags_t explicit_modifiers);
int zmk_hid_implicit_modifiers_press(zmk_mod_flags_t implicit_modifiers);
int zmk_hid_implicit_modifiers_release();
int zmk_hid_keyboard_press(zmk_key_t key);
int zmk_hid_keyboard_release(zmk_key_t key);
void zmk_hid_keyboard_clear();

int zmk_hid_consumer_press(zmk_key_t key);
int zmk_hid_consumer_release(zmk_key_t key);
void zmk_hid_consumer_clear();

struct zmk_hid_keyboard_report *zmk_hid_get_keyboard_report();
struct zmk_hid_consumer_report *zmk_hid_get_consumer_report();
