#pragma once

#include <dt-bindings/zmk/hid_usage.h>

// hid.h defines HID_USAGE_LED_NUM_LOCK as the minimum value, so that is bit
// zero in the report, and all other indicators are relative to that.

#define HID_INDICATOR_NUM_LOCK (1 << (HID_USAGE_LED_NUM_LOCK - HID_USAGE_LED_NUM_LOCK))
#define HID_INDICATOR_CAPS_LOCK (1 << (HID_USAGE_LED_CAPS_LOCK - HID_USAGE_LED_NUM_LOCK))
#define HID_INDICATOR_SCROLL_LOCK (1 << (HID_USAGE_LED_SCROLL_LOCK - HID_USAGE_LED_NUM_LOCK))
#define HID_INDICATOR_COMPOSE (1 << (HID_USAGE_LED_COMPOSE - HID_USAGE_LED_NUM_LOCK))
#define HID_INDICATOR_KANA (1 << (HID_USAGE_LED_KANA - HID_USAGE_LED_NUM_LOCK))
