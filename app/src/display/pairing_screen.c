/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>

#include <zmk/display/widgets/ble_passkey.h>

static struct zmk_widget_ble_passkey passkey_widget;

lv_obj_t *zmk_display_pairing_screen(void) {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);

    zmk_widget_ble_passkey_init(&passkey_widget, screen);

    lv_obj_t *widget = zmk_widget_ble_passkey_obj(&passkey_widget);
    lv_obj_set_width(widget, LV_PCT(100));
    lv_obj_set_height(widget, LV_PCT(100));

    return screen;
}