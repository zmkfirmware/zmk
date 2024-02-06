/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zmk/display.h>
#include <zmk/display/widgets/label.h>

int zmk_widget_label_init(struct zmk_widget_label *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    lv_label_set_text(widget->obj, CONFIG_ZMK_WIDGET_LABEL_TEXT);

    return 0;
}

lv_obj_t *zmk_widget_label_obj(struct zmk_widget_label *widget) { return widget->obj; }
