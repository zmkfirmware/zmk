/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>

#include "hid_indicators_status.h"

#define HID_LED_NUMLOCK_BIT BIT(0)
#define HID_LED_CAPSLOCK_BIT BIT(1)
#define HID_LED_SCROLLLOCK_BIT BIT(2)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct hid_indicators_status_state {
    zmk_hid_indicators_t indicators;
};

static void set_hid_indicators_text(lv_obj_t *label, struct hid_indicators_status_state state) {
    char text[16] = {};
    size_t offset = 0;

    if ((state.indicators & HID_LED_NUMLOCK_BIT) != 0U) {
        offset += snprintf(text + offset, sizeof(text) - offset, "NUM");
    }

    if ((state.indicators & HID_LED_CAPSLOCK_BIT) != 0U && offset < sizeof(text)) {
        offset += snprintf(text + offset, sizeof(text) - offset, "%sCAP", offset > 0U ? " " : "");
    }

    if ((state.indicators & HID_LED_SCROLLLOCK_BIT) != 0U && offset < sizeof(text)) {
        offset += snprintf(text + offset, sizeof(text) - offset, "%sSCR", offset > 0U ? " " : "");
    }

    lv_label_set_text(label, offset > 0U ? text : " ");
}

static void hid_indicators_status_update_cb(struct hid_indicators_status_state state) {
    struct zmk_widget_hid_indicators_status *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_hid_indicators_text(widget->obj, state);
    }
}

static struct hid_indicators_status_state hid_indicators_status_get_state(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *ev = eh ? as_zmk_hid_indicators_changed(eh) : NULL;

    return (struct hid_indicators_status_state){
        .indicators = (ev != NULL) ? ev->indicators : zmk_hid_indicators_get_current_profile(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_hid_indicators_status, struct hid_indicators_status_state,
                            hid_indicators_status_update_cb, hid_indicators_status_get_state)

ZMK_SUBSCRIPTION(widget_hid_indicators_status, zmk_hid_indicators_changed);

int zmk_widget_hid_indicators_status_init(struct zmk_widget_hid_indicators_status *widget,
                                          lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    sys_slist_append(&widgets, &widget->node);

    widget_hid_indicators_status_init();
    return 0;
}

lv_obj_t *zmk_widget_hid_indicators_status_obj(struct zmk_widget_hid_indicators_status *widget) {
    return widget->obj;
}
