/*
 *
 * Copyright (c) 2021 Darryl deHaan
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_peripheral_status {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_widget_peripheral_status_init(struct zmk_widget_peripheral_status *widget,
                                      lv_obj_t *parent);
lv_obj_t *zmk_widget_peripheral_status_obj(struct zmk_widget_peripheral_status *widget);
