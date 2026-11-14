/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#define LM_RGB(r, g, b) ((((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF))

/* Dynamic colors */
#define LM_RAINBOW 0x01000000U
#define LM_PALETTE 0x01000002U

/* Base colors */
#define LM_BLACK LM_RGB(0x00, 0x00, 0x00)
#define LM_WHITE LM_RGB(0xFF, 0xFF, 0xFF)
#define LM_RED LM_RGB(0xFF, 0x00, 0x00)
#define LM_GREEN LM_RGB(0x00, 0xFF, 0x00)
#define LM_BLUE LM_RGB(0x00, 0x00, 0xFF)
#define LM_YELLOW LM_RGB(0xFF, 0xFF, 0x00)
#define LM_CYAN LM_RGB(0x00, 0xFF, 0xFF)
#define LM_MAGENTA LM_RGB(0xFF, 0x00, 0xFF)
#define LM_ORANGE LM_RGB(0xFF, 0x80, 0x00)
#define LM_AMBER LM_RGB(0xFF, 0xBF, 0x00)
#define LM_PURPLE LM_RGB(0x80, 0x00, 0x80)
#define LM_PINK LM_RGB(0xFF, 0x40, 0x80)

/* Neutral gray scale */
#define LM_GRAY_08 LM_RGB(0x08, 0x08, 0x08)
#define LM_GRAY_10 LM_RGB(0x10, 0x10, 0x10)
#define LM_GRAY_20 LM_RGB(0x20, 0x20, 0x20)
#define LM_GRAY_40 LM_RGB(0x40, 0x40, 0x40)
#define LM_GRAY_80 LM_RGB(0x80, 0x80, 0x80)

/* Dim colors */
#define LM_DIM_WHITE LM_RGB(0x10, 0x10, 0x10)
#define LM_DIM_RED LM_RGB(0x10, 0x00, 0x00)
#define LM_DIM_GREEN LM_RGB(0x00, 0x10, 0x00)
#define LM_DIM_BLUE LM_RGB(0x00, 0x00, 0x10)
#define LM_DIM_YELLOW LM_RGB(0x10, 0x10, 0x00)
#define LM_DIM_CYAN LM_RGB(0x00, 0x10, 0x10)
#define LM_DIM_AMBER LM_RGB(0x10, 0x0C, 0x00)

/* Base layer semantic aliases */
#define LM_BASE_OFF LM_BLACK
#define LM_BASE_IDLE LM_DIM_WHITE
#define LM_BASE_SOFT LM_GRAY_20

/* USB semantic aliases */
#define LM_USB_HID LM_GREEN
#define LM_USB_POWERED LM_DIM_WHITE
#define LM_USB_NONE LM_BLACK

/* BLE semantic aliases */
#define LM_BLE_OPEN LM_BLUE
#define LM_BLE_CONNECTED LM_CYAN
#define LM_BLE_DISCONNECTED LM_RED

/* Battery semantic aliases */
#define LM_BATTERY_ON LM_GREEN
#define LM_BATTERY_OFF LM_BLACK
#define LM_BATTERY_LOW LM_RED
#define LM_BATTERY_MID LM_YELLOW
#define LM_BATTERY_HIGH LM_GREEN

/* HID semantic aliases */
#define LM_HID_ON LM_WHITE
#define LM_HID_OFF LM_BLACK
#define LM_HID_CAPS_ON LM_WHITE
#define LM_HID_NUM_ON LM_WHITE
#define LM_HID_SCROLL_ON LM_WHITE
