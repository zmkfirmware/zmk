/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <kernel.h>

struct zmk_widget_bongo_cat {
    sys_snode_t node;
    lv_obj_t *obj;
#if !IS_ENABLED(CONFIG_ZMK_WIDGET_BONGO_CAT_INTERACTIVE)
    lv_anim_t anim;
#endif
};

int zmk_widget_bongo_cat_init(struct zmk_widget_bongo_cat *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_bongo_cat_obj(struct zmk_widget_bongo_cat *widget);
