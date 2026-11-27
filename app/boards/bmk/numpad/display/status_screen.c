/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "status_screen.h"
#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_NUMPAD_CALCULATOR)
#include "../calculator/widget.h"
#endif

#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/output_status.h>

#include "widgets/hid_indicators_status.h"
#include "widgets/led_matrix_effect_status.h"

#define NUMPAD_MID_LABEL_WIDTH 76

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
static struct zmk_widget_battery_status battery_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
static struct zmk_widget_output_status output_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
static struct zmk_widget_layer_status layer_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
static struct zmk_widget_hid_indicators_status hid_indicators_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LED_MATRIX_EFFECT_STATUS)
static struct zmk_widget_led_matrix_effect_status led_matrix_effect_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_INDICATORS_STATUS)
static struct zmk_widget_hid_indicators_status indicators_status_widget;
#endif

lv_obj_t *numpad_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    zmk_widget_hid_indicators_status_init(&hid_indicators_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_hid_indicators_status_obj(&hid_indicators_widget),
                               lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_set_style_text_align(zmk_widget_hid_indicators_status_obj(&hid_indicators_widget),
                                LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_width(zmk_widget_hid_indicators_status_obj(&hid_indicators_widget),
                     NUMPAD_MID_LABEL_WIDTH);
    lv_obj_align(zmk_widget_hid_indicators_status_obj(&hid_indicators_widget), LV_ALIGN_TOP_MID, 0,
                 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);
#endif


#if IS_ENABLED(CONFIG_ZMK_WIDGET_LED_MATRIX_EFFECT_STATUS)
    zmk_widget_led_matrix_effect_status_init(&led_matrix_effect_status_widget, screen);
    lv_obj_set_style_text_font(
        zmk_widget_led_matrix_effect_status_obj(&led_matrix_effect_status_widget),
        lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_set_style_text_align(
        zmk_widget_led_matrix_effect_status_obj(&led_matrix_effect_status_widget),
        LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align(zmk_widget_led_matrix_effect_status_obj(&led_matrix_effect_status_widget),
                 LV_ALIGN_BOTTOM_RIGHT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_NUMPAD_CALCULATOR)
    lv_obj_t *indicators = NULL;
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    indicators = zmk_widget_hid_indicators_status_obj(&hid_indicators_widget);
#endif
    numpad_calculator_widget_init(screen, indicators);
#endif
    return screen;
}
