/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/bongo_cat.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

enum bongo_state {
    bongo_state_none,  /* no hands down */
    bongo_state_left,  /* left hand down */
    bongo_state_right, /* right hand down */
} current_bongo_state;

LV_IMG_DECLARE(none);
LV_IMG_DECLARE(left);
LV_IMG_DECLARE(right);
LV_IMG_DECLARE(both);

const void *images[] = {
    &none,
    &left,
    &right,
    &both
};

int zmk_widget_bongo_cat_init(struct zmk_widget_bongo_cat *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent, NULL);
    lv_img_set_src(widget->obj, &none);
    current_bongo_state = bongo_state_none;

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_bongo_cat_obj(struct zmk_widget_bongo_cat *widget) {
    return widget->obj;
}

void set_bongo_state(struct zmk_widget_bongo_cat *widget, struct zmk_position_state_changed *ev) {
    if (ev == NULL) {
        return;
    }

    uint8_t tmp = bongo_state_left << widget->is_right;
    if (ev->state) {
        if (current_bongo_state & (bongo_state_left | bongo_state_right)) {
            tmp = bongo_state_left | bongo_state_right;
        }
    } else {
        if (current_bongo_state ^ (bongo_state_left | bongo_state_right)) {
            tmp = bongo_state_none;
            widget->is_right = !widget->is_right; /* switch hand when return to none state */
        }
    }

    if (current_bongo_state == tmp) {
        return;
    }

    current_bongo_state = tmp;
    lv_img_set_src(widget->obj, images[current_bongo_state]);
}

int bongo_cat_listener(const zmk_event_t *eh) {
    struct zmk_widget_bongo_cat *widget;
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_bongo_state(widget, ev); }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_bongo_cat, bongo_cat_listener)
ZMK_SUBSCRIPTION(widget_bongo_cat, zmk_position_state_changed);
