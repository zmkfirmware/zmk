/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/led_matrix_state_changed.h>
#include <zmk/led_matrix.h>

#include "led_matrix_effect_status.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

#define LED_MATRIX_EFFECT_STATUS_TEXT_SIZE 96

struct led_matrix_effect_status_state {
    char text[LED_MATRIX_EFFECT_STATUS_TEXT_SIZE];
};

static const char *led_matrix_effect_status_base_text(void) {
    struct zmk_led_matrix_state state;

    if (zmk_led_matrix_get_state(&state) != 0) {
        return "LED";
    }

    if (!state.enabled) {
        return "LED Off";
    }

    if (state.selected_profile_name == NULL || strlen(state.selected_profile_name) == 0U) {
        return "LED";
    }

    return state.selected_profile_name;
}

static void format_led_matrix_effect_status_text(char *text, size_t text_size) {
    snprintf(text, text_size, LV_SYMBOL_LIST " %s", led_matrix_effect_status_base_text());
}

static void set_effect_name(lv_obj_t *label, struct led_matrix_effect_status_state state) {
    lv_label_set_text(label, state.text);
}

static void led_matrix_effect_status_update_cb(struct led_matrix_effect_status_state state) {
    struct zmk_widget_led_matrix_effect_status *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_effect_name(widget->obj, state); }
}

static struct led_matrix_effect_status_state
led_matrix_effect_status_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);

    struct led_matrix_effect_status_state state = {0};

    format_led_matrix_effect_status_text(state.text, sizeof(state.text));

    return state;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_led_matrix_effect_status,
                            struct led_matrix_effect_status_state,
                            led_matrix_effect_status_update_cb,
                            led_matrix_effect_status_get_state)

ZMK_SUBSCRIPTION(widget_led_matrix_effect_status, zmk_led_matrix_state_changed);

int zmk_widget_led_matrix_effect_status_init(struct zmk_widget_led_matrix_effect_status *widget,
                                             lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    sys_slist_append(&widgets, &widget->node);

    widget_led_matrix_effect_status_init();
    return 0;
}

lv_obj_t *
zmk_widget_led_matrix_effect_status_obj(struct zmk_widget_led_matrix_effect_status *widget) {
    return widget->obj;
}
