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

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

LV_IMG_DECLARE(zenlogo);
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
    screen = lv_obj_create(NULL, NULL);

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_BATTERY_STATUS)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), NULL, LV_ALIGN_IN_TOP_MID,
                 0, 2);
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_OUTPUT_STATUS)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), NULL, LV_ALIGN_IN_TOP_MID, 0,
                 41);
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_PERIPHERAL_STATUS)
    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_status_widget), NULL,
                 LV_ALIGN_IN_TOP_MID, 0, 41);
#endif

#if IS_ENABLED(CONFIG_CUSTOM_WIDGET_LAYER_STATUS)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_local_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                                     LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                     lv_theme_get_font_small());
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), NULL, LV_ALIGN_IN_BOTTOM_MID, 0,
                 -5);

    lv_obj_t *LayersHeading;
    LayersHeading = lv_img_create(screen, NULL);
    lv_obj_align(LayersHeading, NULL, LV_ALIGN_IN_BOTTOM_MID, 8, 5);
    lv_img_set_src(LayersHeading, &layers2);
#endif

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    lv_obj_t *zenlogo_icon;
    zenlogo_icon = lv_img_create(screen, NULL);
    lv_img_set_src(zenlogo_icon, &zenlogo);
    lv_obj_align(zenlogo_icon, NULL, LV_ALIGN_IN_BOTTOM_MID, 2, -5);
#endif

    // lv_task_handler();
    lv_refr_now(NULL);
    // display_blanking_off(display_dev);

    return screen;
}
