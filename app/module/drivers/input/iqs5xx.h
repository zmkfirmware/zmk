/*
 * Copyright (c) 2025 Mariano Uvalle
 * SPDX-License-Identifier: MIT
 */

#ifndef ZEPHYR_DRIVERS_INPUT_IQS5XX_H_
#define ZEPHYR_DRIVERS_INPUT_IQS5XX_H_

#include <zephyr/device.h>

#define IQS5XX_NUM_FINGERS 0x0011
#define IQS5XX_REL_X 0x0012          // 2 bytes.
#define IQS5XX_REL_Y 0x0014          // 2 bytes.
#define IQS5XX_ABS_X 0x0016          // 2 bytes.
#define IQS5XX_ABS_Y 0x0018          // 2 bytes.
#define IQS5XX_TOUCH_STRENGTH 0x001A // 2 bytes.
#define IQS5XX_TOUCH_AREA 0x001C

#define IQS5XX_BOTTOM_BETA 0x0637
#define IQS5XX_STATIONARY_THRESH 0x0672

#define IQS5XX_END_COMM_WINDOW 0xEEEE

#define IQS5XX_SYSTEM_CONTROL_0 0x0431
// System Control 0 bits.
#define IQS5XX_ACK_RESET BIT(7)
#define IQS5XX_AUTO_ATI BIT(5)
#define IQS5XX_ALP_RESEED BIT(4)
#define IQS5XX_RESEED BIT(3)

#define IQS5XX_SYSTEM_CONFIG_0 0x058E
// System Config 0 bits.
#define IQS5XX_MANUAL_CONTROL BIT(7)
#define IQS5XX_SETUP_COMPLETE BIT(6)
#define IQS5XX_WDT BIT(5)
#define IQS5XX_SW_INPUT_EVENT BIT(4)
#define IQS5XX_ALP_REATI BIT(3)
#define IQS5XX_REATI BIT(2)
#define IQS5XX_SW_INPUT_SELECT BIT(1)
#define IQS5XX_SW_INPUT BIT(0)

#define IQS5XX_SYSTEM_CONFIG_1 0x058F
// System Config 1 bits.
#define IQS5XX_EVENT_MODE BIT(0)
#define IQS5XX_GESTURE_EVENT BIT(1)
#define IQS5XX_TP_EVENT BIT(2)
#define IQS5XX_REATI_EVENT BIT(3)
#define IQS5XX_ALP_PROX_EVENT BIT(4)
#define IQS5XX_SNAP_EVENT BIT(5)
#define IQS5XX_TOUCH_EVENT BIT(6)
#define IQS5XX_PROX_EVENT BIT(7)

// Filter settings register.
#define IQS5XX_FILTER_SETTINGS 0x0632
// Filter settings bits.
#define IQS5XX_IIR_FILTER BIT(0)
#define IQS5XX_MAV_FILTER BIT(1)
#define IQS5XX_IIR_SELECT BIT(2)
#define IQS5XX_ALP_COUNT_FILTER BIT(3)

#define IQS5XX_SYSTEM_INFO_0 0x000F
// System Info 0 bits.
#define IQS5XX_SHOW_RESET BIT(7)
#define IQS5XX_ALP_REATI_OCCURRED BIT(6)
#define IQS5XX_ALP_ATI_ERROR BIT(5)
#define IQS5XX_REATI_OCCURRED BIT(4)
#define IQS5XX_ATI_ERROR BIT(3)

#define IQS5XX_SYSTEM_INFO_1 0x0010
// System Info 1 bits.
#define IQS5XX_SWITCH_STATE BIT(5)
#define IQS5XX_SNAP_TOGGLE BIT(4)
#define IQS5XX_RR_MISSED BIT(3)
#define IQS5XX_TOO_MANY_FINGERS BIT(2)
#define IQS5XX_PALM_DETECT BIT(1)
#define IQS5XX_TP_MOVEMENT BIT(0)

// These 2 registers have the same bit map.
// The first one configures the gestures,
// the second one reports gesture events at runtime.
#define IQS5XX_SINGLE_FINGER_GESTURES_CONF 0x06B7
#define IQS5XX_GESTURE_EVENTS_0 0x000D
// Single finger gesture identifiers.
#define IQS5XX_SINGLE_TAP BIT(0)
#define IQS5XX_PRESS_AND_HOLD BIT(1)
#define IQS5XX_SWIPE_LEFT BIT(2)
#define IQS5XX_SWIPE_RIGHT BIT(3)
#define IQS5XX_SWIPE_UP BIT(4)
#define IQS5XX_SWIPE_DOWN BIT(5)

// Time in ms, 2 registers wide.
// Hold time + tap time is used as
// a threshold for the press and
// hold gesture.
#define IQS5XX_HOLD_TIME 0x06BD
// TODO: Make hold time configurable with KConfig.

// Mouse button helpers.
#define LEFT_BUTTON_BIT BIT(0)
#define RIGHT_BUTTON_BIT BIT(1)
#define MIDDLE_BUTTON_BIT BIT(2)
#define LEFT_BUTTON_CODE INPUT_BTN_0
#define RIGHT_BUTTON_CODE INPUT_BTN_0 + 1
#define MIDDLE_BUTTON_CODE INPUT_BTN_0 + 2

// These 2 registers have the same bit map.
// The first one configures the gestures,
// the second one reports gesture events at runtime.
#define IQS5XX_MULTI_FINGER_GESTURES_CONF 0x06B8
#define IQS5XX_GESTURE_EVENTS_1 0x000E
// Multi finger gesture identifiers.
#define IQS5XX_TWO_FINGER_TAP BIT(0)
#define IQS5XX_SCROLL BIT(1)
#define IQS5XX_ZOOM BIT(2)

// Axes configuration.
#define IQS5XX_XY_CONFIG_0 0x0669
#define IQS5XX_FLIP_X BIT(0)
#define IQS5XX_FLIP_Y BIT(1)
#define IQS5XX_SWITCH_XY_AXIS BIT(2)

struct iqs5xx_config {
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec rdy_gpio;
    struct gpio_dt_spec reset_gpio;

    // Gesture configuration.
    bool one_finger_tap;
    bool press_and_hold;
    bool two_finger_tap;
    uint16_t press_and_hold_time;

    // Scrolling configuration.
    bool scroll;
    bool natural_scroll_x;
    bool natural_scroll_y;

    // Axes configuration.
    bool switch_xy;
    bool flip_x;
    bool flip_y;

    // Sensitivity. configuration.
    uint8_t bottom_beta;
    uint8_t stationary_threshold;
};

struct iqs5xx_data {
    const struct device *dev;
    struct gpio_callback rdy_cb;
    struct k_work work;
    struct k_work_delayable button_release_work;
    // TODO: Pack flags into a bitfield to save space.
    bool initialized;
    // Flag to indicate if the button was pressed in a previous cycle.
    uint8_t buttons_pressed;
    bool active_hold;
    // Scroll accumulators.
    int16_t scroll_x_acc;
    int16_t scroll_y_acc;
};

#endif /* ZEPHYR_DRIVERS_INPUT_IQS5XX_H_ */
