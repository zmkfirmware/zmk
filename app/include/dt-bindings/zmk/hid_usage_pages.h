/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Based on HID Usage Tables 1.21,
 * Copyright © 1996-2020, USB Implementers Forum,
 * https://www.usb.org/sites/default/files/hut1_21.pdf
 */

#pragma once

#define HID_USAGE(page, id) ((page << 16) | id)
#define HID_USAGE_ID(usage) (usage & 0xFFFF)
#define HID_USAGE_PAGE(usage) (usage >> 16)

/* WARNING: DEPRECATED from dt-bindings/zmk/keys.h */
#define USAGE_KEYPAD (0x07)   // WARNING: DEPRECATED (DO NOT USE)
#define USAGE_CONSUMER (0x0C) // WARNING: DEPRECATED (DO NOT USE)

#define HID_USAGE_GD (0x01)             // Generic Desktop
#define HID_USAGE_SIM (0x02)            // Simulation Controls
#define HID_USAGE_VR (0x03)             // VR Controls
#define HID_USAGE_SPORT (0x04)          // Sport Controls
#define HID_USAGE_GAME (0x05)           // Game Controls
#define HID_USAGE_GDV (0x06)            // Generic Device Controls
#define HID_USAGE_KEY (0x07)            // Keyboard/Keypad
#define HID_USAGE_LED (0x08)            // LED
#define HID_USAGE_BUTTON (0x09)         // Button
#define HID_USAGE_TELEPHONY (0x0B)      // Telephony Device
#define HID_USAGE_CONSUMER (0x0C)       // Consumer
#define HID_USAGE_DIGITIZERS (0x0D)     // Digitizers
#define HID_USAGE_HAPTICS (0x0E)        // Haptics
#define HID_USAGE_PID (0x0F)            // PID
#define HID_USAGE_EHT (0x12)            // Eye and Head Trackers
#define HID_USAGE_AUXDISP (0x14)        // Auxiliary Display
#define HID_USAGE_SENSORS (0x20)        // Sensors
#define HID_USAGE_MEDICAL (0x40)        // Medical Instrument
#define HID_USAGE_BRAILLE (0x41)        // Braille Display
#define HID_USAGE_LIGHT (0x59)          // Lighting And Illumination
#define HID_USAGE_MONITOR (0x80)        // USB Monitor
#define HID_USAGE_MONITOR_VALUES (0x81) // Monitor Enumerated Values
#define HID_USAGE_MONITOR_VESA (0x82)   // VESA Virtual Control
#define HID_USAGE_POWER (0x84)          // Power
#define HID_USAGE_POS_BARCODE (0x8C)    // Bar Code Scanner
#define HID_USAGE_POS_SCALE (0x8D)      // Scale
#define HID_USAGE_POS_MSR (0x8E)        // Magnetic Stripe Reading (MSR) Devices
#define HID_USAGE_POS_RESV (0x8F)       // Reserved Point of Sale
#define HID_USAGE_CAMERA (0x90)         // Camera Control
#define HID_USAGE_ARCADE (0x91)         // Arcade
#define HID_USAGE_GAMING (0x92)         // Gaming Device
#define HID_USAGE_FIDO (0xF1D0)         // FIDO Alliance