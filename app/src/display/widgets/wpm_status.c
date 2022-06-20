/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/wpm_status.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/wpm.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct wpm_status_state {
    uint8_t wpm;
};

struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    return (struct wpm_status_state){.wpm = zmk_wpm_get_state()};
};

void set_wpm_symbol(lv_obj_t *obj, struct wpm_status_state state) {
    char text[4] = {};

    LOG_DBG("WPM changed to %i", state.wpm);
    snprintf(text, sizeof(text), "%i", state.wpm);

    lv_span_t *span = lv_spangroup_get_child(obj, 0);
    lv_span_set_text(span, text);
}

void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_wpm_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_symbol(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent) {
    widget->obj = lv_spangroup_create(parent);
    lv_spangroup_new_span(widget->obj);
    lv_spangroup_set_align(widget->obj, LV_TEXT_ALIGN_RIGHT);
    lv_spangroup_set_mode(widget->obj, LV_SPAN_MODE_EXPAND);
    lv_obj_set_size(widget->obj, 60, 32);

    sys_slist_append(&widgets, &widget->node);

    widget_wpm_status_init();
    return 0;
}

lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget) { return widget->obj; }
