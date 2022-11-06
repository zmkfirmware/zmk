/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <kernel.h>

struct zmk_widget_luna {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_anim_t anim;
};

int zmk_widget_luna_init(struct zmk_widget_luna *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_luna_obj(struct zmk_widget_luna *widget);
