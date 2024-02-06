/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_label {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_widget_label_init(struct zmk_widget_label *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_label_obj(struct zmk_widget_label *widget);
