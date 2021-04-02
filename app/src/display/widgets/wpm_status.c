/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display/widgets/wpm_status.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/wpm.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

void set_wpm_symbol(lv_obj_t *label, int wpm) {
    char text[4] = {};

    LOG_DBG("WPM changed to %i", wpm);
    sprintf(text, "%i ", wpm);

    lv_label_set_text(label, text);
}

int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent, NULL);
    lv_label_set_align(widget->obj, LV_LABEL_ALIGN_RIGHT);

    lv_obj_set_size(widget->obj, 40, 15);
    set_wpm_symbol(widget->obj, 0);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget) { return widget->obj; }

int wpm_status_listener(const zmk_event_t *eh) {
    struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    struct zmk_widget_wpm_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_symbol(widget->obj, ev->state); }
    return 0;
}

ZMK_LISTENER(widget_wpm_status, wpm_status_listener)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);