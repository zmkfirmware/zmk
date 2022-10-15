/*
 *
 * Copyright (c) 2021 Darryl deHaan
 * SPDX-License-Identifier: MIT
 *
 */

#include "widgets/battery_status.h"
#include "widgets/peripheral_status.h"
#include "widgets/output_status.h"
#include "widgets/layer_status.h"
#include "custom_status_screen.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

LV_IMG_DECLARE(layers2);

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_BATTERY_STATUS)
static struct zmk_widget_battery_status battery_status_widget;
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_OUTPUT_STATUS)
static struct zmk_widget_output_status output_status_widget;
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_PERIPHERAL_STATUS)
static struct zmk_widget_peripheral_status peripheral_status_widget;
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS)
static struct zmk_widget_layer_status layer_status_widget;
#endif

lv_obj_t *zmk_display_status_screen() {

    lv_obj_t *screen;
    screen = lv_obj_create(NULL);

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_BATTERY_STATUS)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS_HIDE_HEADING)
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_CENTER, 0, -43);
#else
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_MID, 0, 2);
#endif
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_OUTPUT_STATUS)
    zmk_widget_output_status_init(&output_status_widget, screen);
#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS_HIDE_HEADING)
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_CENTER, 0, 0);
#else
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_MID, 0, 41);
#endif
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_PERIPHERAL_STATUS)
    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS_HIDE_HEADING)
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_status_widget), LV_ALIGN_CENTER, 0,
                 0);
#else
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_status_widget), LV_ALIGN_TOP_MID, 0,
                 41);
#endif
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS)
#if !IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS_HIDE_HEADING)
    lv_obj_t *LayersHeading;
    LayersHeading = lv_img_create(screen);
    lv_obj_align(LayersHeading, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_img_set_src(LayersHeading, &layers2);
#endif

    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               &lv_font_montserrat_16, LV_PART_MAIN);
#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS_HIDE_HEADING)
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_CENTER, 0, 43);
#else
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_MID, 0, -5);
#endif
#endif

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    lv_obj_t *zenlogo_icon;
    zenlogo_icon = lv_img_create(screen);
  #if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LOGO_IMAGE_ZEN)
    LV_IMG_DECLARE(zenlogo);
    lv_img_set_src(zenlogo_icon, &zenlogo);
  #elif IS_ENABLED(CONFIG_CUSTOM_WIDGET_LOGO_IMAGE_LPKB)
    LV_IMG_DECLARE(lpkblogo);
    lv_img_set_src(zenlogo_icon, &lpkblogo);
  #elif IS_ENABLED(CONFIG_CUSTOM_WIDGET_LOGO_IMAGE_ZMK)
    LV_IMG_DECLARE(zmklogo);
    lv_img_set_src(zenlogo_icon, &zmklogo);
  #elif IS_ENABLED(CONFIG_CUSTOM_WIDGET_LOGO_IMAGE_MIRYOKU)
    LV_IMG_DECLARE(miryokulogo);
    lv_img_set_src(zenlogo_icon, &miryokulogo);
  #endif
#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS_HIDE_HEADING)
    lv_obj_align(zenlogo_icon, LV_ALIGN_CENTER, 0, 43);
#else
    lv_obj_align(zenlogo_icon, LV_ALIGN_BOTTOM_MID, 0, -5);
#endif
#endif

    return screen;
}
