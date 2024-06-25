/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/mouse/hid.h>

static struct zmk_hid_mouse_report mouse_report = {
    .report_id = ZMK_MOUSE_HID_REPORT_ID_MOUSE,
    .body = {.buttons = 0, .d_x = 0, .d_y = 0, .d_scroll_y = 0}};

// Keep track of how often a button was pressed.
// Only release the button if the count is 0.
static int explicit_button_counts[5] = {0, 0, 0, 0, 0};
static zmk_mod_flags_t explicit_buttons = 0;

#define SET_MOUSE_BUTTONS(btns)                                                                    \
    {                                                                                              \
        mouse_report.body.buttons = btns;                                                          \
        LOG_DBG("Mouse buttons set to 0x%02X", mouse_report.body.buttons);                         \
    }

int zmk_hid_mouse_button_press(zmk_mouse_button_t button) {
    if (button >= ZMK_MOUSE_HID_NUM_BUTTONS) {
        return -EINVAL;
    }

    explicit_button_counts[button]++;
    LOG_DBG("Button %d count %d", button, explicit_button_counts[button]);
    WRITE_BIT(explicit_buttons, button, true);
    SET_MOUSE_BUTTONS(explicit_buttons);
    return 0;
}

int zmk_hid_mouse_button_release(zmk_mouse_button_t button) {
    if (button >= ZMK_MOUSE_HID_NUM_BUTTONS) {
        return -EINVAL;
    }

    if (explicit_button_counts[button] <= 0) {
        LOG_ERR("Tried to release button %d too often", button);
        return -EINVAL;
    }
    explicit_button_counts[button]--;
    LOG_DBG("Button %d count: %d", button, explicit_button_counts[button]);
    if (explicit_button_counts[button] == 0) {
        LOG_DBG("Button %d released", button);
        WRITE_BIT(explicit_buttons, button, false);
    }
    SET_MOUSE_BUTTONS(explicit_buttons);
    return 0;
}

int zmk_hid_mouse_buttons_press(zmk_mouse_button_flags_t buttons) {
    for (zmk_mouse_button_t i = 0; i < ZMK_MOUSE_HID_NUM_BUTTONS; i++) {
        if (buttons & BIT(i)) {
            zmk_hid_mouse_button_press(i);
        }
    }
    return 0;
}

int zmk_hid_mouse_buttons_release(zmk_mouse_button_flags_t buttons) {
    for (zmk_mouse_button_t i = 0; i < ZMK_MOUSE_HID_NUM_BUTTONS; i++) {
        if (buttons & BIT(i)) {
            zmk_hid_mouse_button_release(i);
        }
    }
    return 0;
}

void zmk_hid_mouse_movement_set(int16_t x, int16_t y) {
    mouse_report.body.d_x = x;
    mouse_report.body.d_y = y;
    LOG_DBG("Mouse movement set to %d/%d", mouse_report.body.d_x, mouse_report.body.d_y);
}

void zmk_hid_mouse_movement_update(int16_t x, int16_t y) {
    mouse_report.body.d_x += x;
    mouse_report.body.d_y += y;
    LOG_DBG("Mouse movement updated to %d/%d", mouse_report.body.d_x, mouse_report.body.d_y);
}

void zmk_hid_mouse_scroll_set(int8_t x, int8_t y) {
    mouse_report.body.d_scroll_x = x;
    mouse_report.body.d_scroll_y = y;
    LOG_DBG("Mouse scroll set to %d/%d", mouse_report.body.d_scroll_x,
            mouse_report.body.d_scroll_y);
}

void zmk_hid_mouse_scroll_update(int8_t x, int8_t y) {
    mouse_report.body.d_scroll_x += x;
    mouse_report.body.d_scroll_y += y;
    LOG_DBG("Mouse scroll updated to X: %d/%d", mouse_report.body.d_scroll_x,
            mouse_report.body.d_scroll_y);
}

void zmk_hid_mouse_clear(void) {
    LOG_DBG("Mouse report cleared");
    memset(&mouse_report.body, 0, sizeof(mouse_report.body));
}

struct zmk_hid_mouse_report *zmk_mouse_hid_get_mouse_report(void) {
    return &mouse_report;
}
