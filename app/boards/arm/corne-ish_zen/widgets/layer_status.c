/*
*
* Copyright (c) 2021 Darryl deHaan
* SPDX-License-Identifier: MIT
*
*/

#include <kernel.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include "layer_status.h"
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static lv_style_t label_style;

static bool style_initialized = false;

K_MUTEX_DEFINE(layer_status_mutex);

struct layer_status_state {
    uint8_t index;
    const char *label;
};

void layer_status_init() {
    if (style_initialized) {
        return;
    }
    style_initialized = true;
    lv_style_init(&label_style);
    lv_style_set_text_color(&label_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_font(&label_style, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_style_set_text_letter_space(&label_style, LV_STATE_DEFAULT, 1);
    lv_style_set_text_line_space(&label_style, LV_STATE_DEFAULT, 1);

}

static void set_layer_symbol(lv_obj_t *label, struct layer_status_state state) {

    //k_mutex_lock(&layer_status_mutex, K_FOREVER);
    const char *layer_label = state.label;
    uint8_t active_layer_index = state.index;
    //k_mutex_unlock(&layer_status_mutex);
    
    if (layer_label == NULL) {
        char text[6] = {};

        sprintf(text, " %i", active_layer_index);

        lv_label_set_text(label, text);
    } else {
        lv_label_set_text(label, layer_label);
    }
}

//void layer_status_update_cb(struct k_work *work) {
static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_symbol(widget->obj, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){.index = index, .label = zmk_keymap_layer_label(index)};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);



int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent) {
    layer_status_init();
    widget->obj = lv_label_create(parent, NULL);

    lv_obj_add_style(widget->obj, LV_LABEL_PART_MAIN, &label_style);

    sys_slist_append(&widgets, &widget->node);

    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget) {
    return widget->obj;
}





/*
static void update_state() {
    k_mutex_lock(&layer_status_mutex, K_FOREVER);
    layer_status_state.index = zmk_keymap_highest_layer_active();
    layer_status_state.label = zmk_keymap_layer_label(layer_status_state.index);
    LOG_DBG("Layer changed to %i", layer_status_state.index);

    k_mutex_unlock(&layer_status_mutex);
}


K_WORK_DEFINE(layer_status_update_work, layer_status_update_cb);

int layer_status_listener(const zmk_event_t *eh) {
    update_state();;

    k_work_submit_to_queue(zmk_display_work_q(), &layer_status_update_work);
    return 0;
}
*/