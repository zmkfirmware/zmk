/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/mods_status.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>

#define MOD_CHARS CONFIG_ZMK_WIDGET_MODS_STATUS_CHARACTERS
#define MOD_CHARS_LEN (sizeof(MOD_CHARS) - 1)

BUILD_ASSERT(MOD_CHARS_LEN == 4,
             "ERROR: CONFIG_ZMK_WIDGET_MODS_STATUS_CHARACTERS should have exactly 4 characters");

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct mods_status_state {
    uint8_t mods;
};

struct mods_status_state mods_status_get_state(const zmk_event_t *eh) {
    return (struct mods_status_state){.mods = zmk_hid_get_explicit_mods()};
};

void set_mods_symbol(lv_obj_t *label, struct mods_status_state state) {
    char text[5] = {};

    LOG_DBG("mods changed to %i", state.mods);
    if (state.mods & (MOD_LCTL | MOD_RCTL))
        strncat(text, &MOD_CHARS[0], 1);
    if (state.mods & (MOD_LSFT | MOD_RSFT))
        strncat(text, &MOD_CHARS[1], 1);
    if (state.mods & (MOD_LALT | MOD_RALT))
        strncat(text, &MOD_CHARS[2], 1);
    if (state.mods & (MOD_LGUI | MOD_RGUI))
        strncat(text, &MOD_CHARS[3], 1);

    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_BOTTOM_RIGHT, -1, 0);
}

void mods_status_update_cb(struct mods_status_state state) {
    struct zmk_widget_mods_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_mods_symbol(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_mods_status, struct mods_status_state, mods_status_update_cb,
                            mods_status_get_state)
ZMK_SUBSCRIPTION(widget_mods_status, zmk_keycode_state_changed);

int zmk_widget_mods_status_init(struct zmk_widget_mods_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_RIGHT, 0);

    sys_slist_append(&widgets, &widget->node);

    widget_mods_status_init();
    return 0;
}

lv_obj_t *zmk_widget_mods_status_obj(struct zmk_widget_mods_status *widget) { return widget->obj; }
