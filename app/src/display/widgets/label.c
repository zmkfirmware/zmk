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
#include <zmk/events/activity_state_changed.h>
#include <zmk/event_manager.h>

#define WIDGET_LABEL_TEXT 7

BUILD_ASSERT(sizeof(CONFIG_ZMK_WIDGET_LABEL_TEXT) - 1 <= WIDGET_LABEL_TEXT,
             "ERROR: Widget label text length is too long. Max length: 7");

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct label_state {};

static struct label_state get_state(const zmk_event_t *_eh) { return (struct label_state){}; }

static void set_label_symbol(lv_obj_t *label) {
    lv_label_set_text(label, CONFIG_ZMK_WIDGET_LABEL_TEXT);
}

static void label_update_cb() {
    struct zmk_widget_label *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_label_symbol(widget->obj); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_label, struct label_state, label_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_label, zmk_activity_state_changed);

int zmk_widget_label_init(struct zmk_widget_label *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    sys_slist_append(&widgets, &widget->node);

    widget_label_init();
    return 0;
}

lv_obj_t *zmk_widget_label_obj(struct zmk_widget_label *widget) { return widget->obj; }
