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

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BONGO_CAT_INTERACTIVE)
#include <zmk/events/position_state_changed.h>
#endif

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

LV_IMG_DECLARE(left);
LV_IMG_DECLARE(right);

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BONGO_CAT_INTERACTIVE)
LV_IMG_DECLARE(none);
LV_IMG_DECLARE(both);

enum bongo_state {
    bongo_state_none,
    bongo_state_left,
    bongo_state_right,
} current_bongo_state;

const void *images[] = {
    &none,
    &left,
    &right,
    &both
};
#else
const void *images[] = {
    &left,
    &right
};

void set_bongo_state(void *var, lv_anim_value_t val) {
    lv_img_set_src( (lv_obj_t *)var, images[val]);
}
#endif /* IS_ENABLED(CONFIG_ZMK_WIDGET_BONGO_CAT_INTERACTIVE) */

int zmk_widget_bongo_cat_init(struct zmk_widget_bongo_cat *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent, NULL);

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BONGO_CAT_INTERACTIVE)
    lv_img_set_src(widget->obj, &none);
    current_bongo_state = bongo_state_none;
#else
    lv_anim_init(&widget->anim);
    lv_anim_set_exec_cb(&widget->anim, (lv_anim_exec_xcb_t) set_bongo_state);
    lv_anim_set_var(&widget->anim, widget->obj);
    lv_anim_set_time(&widget->anim, CONFIG_ZMK_WIDGET_BONGO_CAT_ANIMATION_INTERVAL);
    lv_anim_set_values(&widget->anim, 0, 1);
    lv_anim_set_repeat_count(&widget->anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&widget->anim, CONFIG_ZMK_WIDGET_BONGO_CAT_ANIMATION_INTERVAL);
    lv_anim_start(&widget->anim);
#endif /* IS_ENABLED(CONFIG_ZMK_WIDGET_BONGO_CAT_INTERACTIVE) */

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_bongo_cat_obj(struct zmk_widget_bongo_cat *widget) {
    return widget->obj;
}

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BONGO_CAT_INTERACTIVE)
void set_bongo_state(lv_obj_t *img, struct zmk_position_state_changed *ev) {
    if (ev == NULL) {
        return;
    }

    uint8_t tmp = bongo_state_left;
    if (ev->state) {
        if (current_bongo_state & (bongo_state_left | bongo_state_right)) {
            tmp = bongo_state_left | bongo_state_right;
        }
    } else {
        if (current_bongo_state ^ (bongo_state_left | bongo_state_right)) {
            tmp = bongo_state_none;
        }
    }

    current_bongo_state = tmp;
    lv_img_set_src(img, images[current_bongo_state]);
}

int bongo_cat_listener(const zmk_event_t *eh) {
    struct zmk_widget_bongo_cat *widget;
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_bongo_state(widget->obj, ev); }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_bongo_cat, bongo_cat_listener)
ZMK_SUBSCRIPTION(widget_bongo_cat, zmk_position_state_changed);
#endif /* IS_ENABLED(CONFIG_ZMK_WIDGET_BONGO_CAT_INTERACTIVE) */
