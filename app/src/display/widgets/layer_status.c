/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display/widgets/layer_status.h>
#include <zmk/events/layer-state-changed.h>
#include <zmk/event-manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static lv_style_t label_style;

void layer_status_init() {
    if (label_style.text.font != NULL) {
        return;
    }

    lv_style_copy(&label_style, &lv_style_plain);
    label_style.text.color = LV_COLOR_BLACK;
    label_style.text.font = &lv_font_roboto_12;
    label_style.text.letter_space = 1;
    label_style.text.line_space = 1;
}

void set_layer_symbol(lv_obj_t *label) {
    int active_layer_index = zmk_keymap_highest_layer_active();
    char text[6] = {};

    LOG_DBG("Layer changed to %i", active_layer_index);
    sprintf(text, LV_SYMBOL_KEYBOARD "%i ", active_layer_index);

    lv_label_set_text(label, text);
}

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent) {
    layer_status_init();
    widget->obj = lv_label_create(parent, NULL);
    lv_label_set_style(widget->obj, LV_LABEL_STYLE_MAIN, &label_style);

    lv_obj_set_size(widget->obj, 40, 15);
    set_layer_symbol(widget->obj);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget) {
    return widget->obj;
}

int layer_status_listener(const struct zmk_event_header *eh) {
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_symbol(widget->obj); }
    return 0;
}

ZMK_LISTENER(widget_layer_status, layer_status_listener)
ZMK_SUBSCRIPTION(widget_layer_status, layer_state_changed);