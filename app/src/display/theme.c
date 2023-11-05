/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "theme.h"

#include <lvgl.h>
#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_LV_USE_THEME_MONO)

static lv_theme_t *initialize_theme(lv_disp_t *disp) {
    lv_theme_t *theme = lv_theme_mono_init(disp, IS_ENABLED(CONFIG_ZMK_DISPLAY_INVERT),
                                           CONFIG_ZMK_LV_FONT_DEFAULT_NORMAL);

    theme->font_small = CONFIG_ZMK_LV_FONT_DEFAULT_SMALL;
    theme->font_normal = CONFIG_ZMK_LV_FONT_DEFAULT_NORMAL;
    theme->font_large = CONFIG_ZMK_LV_FONT_DEFAULT_LARGE;

    return theme;
}

#else

// Create a basic theme which only sets font sizes.
static lv_theme_t theme;

static void theme_apply(lv_theme_t *th, lv_obj_t *obj) {
    LV_UNUSED(th);
    LV_UNUSED(obj);
}

static lv_theme_t *initialize_theme(lv_disp_t *disp) {
    theme.disp = disp;
    theme.font_small = CONFIG_ZMK_LV_FONT_DEFAULT_SMALL;
    theme.font_normal = CONFIG_ZMK_LV_FONT_DEFAULT_NORMAL;
    theme.font_large = CONFIG_ZMK_LV_FONT_DEFAULT_LARGE;
    theme.apply_cb = theme_apply;

    return &theme;
}

#endif // IS_ENABLED(CONFIG_LV_USE_THEME_MONO)

void zmk_display_initialize_theme(void) {
    lv_disp_t *disp = lv_disp_get_default();
    lv_theme_t *theme = initialize_theme(disp);
    lv_disp_set_theme(disp, theme);
}