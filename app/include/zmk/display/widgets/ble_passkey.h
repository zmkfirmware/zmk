/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_ble_passkey {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *profile;
    lv_obj_t *title;
    lv_obj_t *passkey;
};

int zmk_widget_ble_passkey_init(struct zmk_widget_ble_passkey *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_ble_passkey_obj(struct zmk_widget_ble_passkey *widget);
