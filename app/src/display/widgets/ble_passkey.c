/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>
#include <zmk/ble/auth.h>
#include <zmk/display.h>
#include <zmk/display/widgets/ble_passkey.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_auth_state_changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void get_passkey_entry_text(char *str, size_t size, const struct zmk_ble_auth_state *state) {
    if (state->cursor_index == 0) {
        snprintf(str, size, "_");
    } else if (state->cursor_index == 6) {
        snprintf(str, size, "%06u " LV_SYMBOL_NEW_LINE, state->passkey);
    } else {
        snprintf(str, size, "%0*u_", state->cursor_index, state->passkey);
    }
}

static void update_passkey_widget(struct zmk_widget_ble_passkey *widget,
                                  const struct zmk_ble_auth_state *state) {
    const int profile_index = state->profile_index + 1;
    lv_label_set_text_fmt(widget->profile, LV_SYMBOL_WIFI " %i", profile_index);

    switch (state->mode) {
    case ZMK_BLE_AUTH_MODE_PASSKEY_CONFIRM:
        lv_label_set_text_static(widget->title, "Confirm PIN");
        lv_label_set_text_fmt(widget->passkey, "%06u " LV_SYMBOL_NEW_LINE, state->passkey);
        break;

    case ZMK_BLE_AUTH_MODE_PASSKEY_ENTRY: {
        char passkey[16];
        lv_label_set_text_static(widget->title, "Enter PIN");
        get_passkey_entry_text(passkey, sizeof(passkey), state);
        lv_label_set_text(widget->passkey, passkey);
    } break;

    case ZMK_BLE_AUTH_MODE_PASSKEY_DISPLAY:
    default:
        lv_label_set_text_static(widget->title, "Pairing PIN");
        lv_label_set_text_fmt(widget->passkey, "%06u", state->passkey);
        break;
    }
}

static void ble_passkey_update_cb(struct zmk_ble_auth_state state) {
    struct zmk_widget_ble_passkey *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { update_passkey_widget(widget, &state); }
}

static struct zmk_ble_auth_state ble_passkey_get_state(const zmk_event_t *eh) {
    return zmk_ble_get_auth_state();
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_ble_passkey, struct zmk_ble_auth_state, ble_passkey_update_cb,
                            ble_passkey_get_state);
ZMK_SUBSCRIPTION(widget_ble_passkey, zmk_ble_auth_state_changed);

static bool is_small_display(void) { return lv_disp_get_ver_res(lv_disp_get_default()) < 40; }

int zmk_widget_ble_passkey_init(struct zmk_widget_ble_passkey *widget, lv_obj_t *parent) {
    static const lv_coord_t col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    sys_slist_append(&widgets, &widget->node);

    const bool is_small = is_small_display();
    const lv_font_t *passkey_font =
        is_small ? lv_theme_get_font_normal(parent) : lv_theme_get_font_large(parent);

    widget->obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(widget->obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_obj_set_grid_dsc_array(widget->obj, col_dsc, row_dsc);
    lv_obj_set_layout(widget->obj, LV_LAYOUT_GRID);

    if (is_small) {
        lv_obj_set_style_pad_all(widget->obj, 0, 0);
    }

    widget->profile = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(widget->profile, lv_theme_get_font_small(parent), LV_PART_MAIN);
    lv_obj_set_grid_cell(widget->profile, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);

    widget->title = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(widget->title, lv_theme_get_font_small(parent), LV_PART_MAIN);
    lv_obj_set_grid_cell(widget->title, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_START, 0, 1);

    widget->passkey = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(widget->passkey, passkey_font, LV_PART_MAIN);
    lv_obj_set_grid_cell(widget->passkey, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 1, 1);

    widget_ble_passkey_init();

    return 0;
}

lv_obj_t *zmk_widget_ble_passkey_obj(struct zmk_widget_ble_passkey *widget) { return widget->obj; }