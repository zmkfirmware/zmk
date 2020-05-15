#pragma once

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <dt-bindings/zmk/keys.h>

#include "keys.h"

#define ZMK_HID_MAX_KEYCODE KC_APP

static const u8_t zmk_hid_report_desc[] = {
    /* USAGE_PAGE (Generic Desktop) */
    HID_GI_USAGE_PAGE,
    USAGE_GEN_DESKTOP,
    /* USAGE (Keyboard) */
    HID_LI_USAGE,
    USAGE_GEN_DESKTOP_KEYBOARD,
    /* COLLECTION (Application) */
    HID_MI_COLLECTION,
    COLLECTION_APPLICATION,
    /* REPORT ID (1) */
    HID_GI_REPORT_ID,
    0x01,
    /* USAGE_PAGE (Keypad) */
    HID_GI_USAGE_PAGE,
    USAGE_GEN_DESKTOP_KEYPAD,
    /* USAGE_MINIMUM (Keyboard LeftControl) */
    HID_LI_USAGE_MIN(1),
    0xE0,
    /* USAGE_MAXIMUM (Keyboard Right GUI) */
    HID_LI_USAGE_MAX(1),
    0xE7,
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

    /* USAGE_PAGE (Keypad) */
    HID_GI_USAGE_PAGE,
    USAGE_GEN_DESKTOP_KEYPAD,
    /* LOGICAL_MINIMUM (0) */
    HID_GI_LOGICAL_MIN(1),
    0x00,
    /* LOGICAL_MAXIMUM (101) */
    HID_GI_LOGICAL_MAX(1),
    0x01,
    /* USAGE_MINIMUM (Reserved) */
    HID_LI_USAGE_MIN(1),
    0x00,
    /* USAGE_MAXIMUM (Keyboard Application) */
    HID_LI_USAGE_MAX(1),
    ZMK_HID_MAX_KEYCODE,
    /* REPORT_SIZE (8) */
    HID_GI_REPORT_SIZE,
    0x01,
    /* REPORT_COUNT (6) */
    HID_GI_REPORT_COUNT,
    ZMK_HID_MAX_KEYCODE + 1,
    /* INPUT (Data,Ary,Abs) */
    HID_MI_INPUT,
    0x02,
    /* USAGE_PAGE (Keypad) */
    HID_GI_USAGE_PAGE,
    USAGE_GEN_DESKTOP_KEYPAD,
    /* REPORT_SIZE (8) */
    HID_GI_REPORT_SIZE,
    0x02,
    /* REPORT_COUNT (6) */
    HID_GI_REPORT_COUNT,
    0x01,
    /* INPUT (Cnst,Var,Abs) */
    HID_MI_INPUT,
    0x03,
    /* END_COLLECTION */
    HID_MI_COLLECTION_END,
};

// struct zmk_hid_boot_report
// {
//     u8_t modifiers;
//     u8_t _unused;
//     u8_t keys[6];
// } __packed;

struct zmk_hid_report
{
    u8_t modifiers;
    u8_t keys[13];
} __packed;

int zmk_hid_press_key(zmk_key key);
int zmk_hid_release_key(zmk_key key);

struct zmk_hid_report *zmk_hid_get_report();