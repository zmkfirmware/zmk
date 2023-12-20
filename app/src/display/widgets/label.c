/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/label.h>

#define WIDGET_LABEL_TEXT_MAX 7

BUILD_ASSERT(sizeof(CONFIG_ZMK_WIDGET_LABEL_TEXT) - 1 <= WIDGET_LABEL_TEXT_MAX,
             "ERROR: Widget label text length is too long. Max length: 7");

int zmk_widget_label_init(struct zmk_widget_label *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    lv_label_set_text(widget->obj, CONFIG_ZMK_WIDGET_LABEL_TEXT);

    return 0;
}

lv_obj_t *zmk_widget_label_obj(struct zmk_widget_label *widget) { return widget->obj; }
