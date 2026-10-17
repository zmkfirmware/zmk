/* SPDX-License-Identifier: MIT */
#include "calculator.h"
#include "widget.h"
#include <zmk/display.h>

static lv_obj_t *title;
static lv_obj_t *body;
static lv_obj_t *expression;
static lv_obj_t *result;
static lv_obj_t *notice;
static lv_obj_t *lock_indicators;

static void update(struct numpad_calculator_state state) {
    if (state.active) {
        lv_obj_clear_flag(title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(body, LV_OBJ_FLAG_HIDDEN);
        if (lock_indicators) {
            lv_obj_add_flag(lock_indicators, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(body, LV_OBJ_FLAG_HIDDEN);
        if (lock_indicators) {
            lv_obj_clear_flag(lock_indicators, LV_OBJ_FLAG_HIDDEN);
        }
    }
    lv_label_set_text(expression, state.engine.expression);
    const char *text = state.engine.evaluated
                           ? state.engine.result
                           : (state.engine.expression[0] ? state.engine.expression : "0");
    lv_label_set_text(result, text);
    const lv_font_t *fonts[] = {&lv_font_montserrat_32, &lv_font_montserrat_24,
                                &lv_font_montserrat_16};
    const lv_font_t *font = fonts[2];
    for (size_t i = 0; i < ARRAY_SIZE(fonts); i++) {
        if (lv_text_get_width(text, strlen(text), fonts[i], 0) <= lv_obj_get_width(result)) {
            font = fonts[i];
            break;
        }
    }
    lv_obj_set_style_text_font(result, font, LV_PART_MAIN);
    lv_label_set_text(notice,
                      state.engine.error ? calc_error_text(state.engine.error) : state.notice);
}

static struct numpad_calculator_state get_state(const zmk_event_t *eh) {
    return numpad_calculator_snapshot();
}
ZMK_DISPLAY_WIDGET_LISTENER(calculator_display, struct numpad_calculator_state, update, get_state)
ZMK_SUBSCRIPTION(calculator_display, numpad_calculator_changed);

void numpad_calculator_widget_init(lv_obj_t *screen, lv_obj_t *indicators) {
    lock_indicators = indicators;
    title = lv_label_create(screen);
    lv_label_set_text(title, "Cal");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    body = lv_obj_create(screen);
    lv_obj_set_size(body, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(body, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(body, 0, LV_PART_MAIN);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    /* Reserve 24 px at both edges for the existing status rows. */
    lv_obj_update_layout(screen);
    lv_obj_set_size(body, lv_obj_get_content_width(screen) - 8,
                    lv_obj_get_content_height(screen) - 48);
    lv_obj_align(body, LV_ALIGN_CENTER, 0, 0);

    expression = lv_label_create(body);
    lv_obj_set_width(expression, lv_pct(100));
    lv_obj_set_style_text_font(expression, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_label_set_long_mode(expression, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(expression, LV_ALIGN_TOP_LEFT, 0, 0);

    result = lv_label_create(body);
    lv_obj_set_width(result, lv_pct(100));
    lv_obj_set_style_text_align(result, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_label_set_long_mode(result, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(result, LV_ALIGN_CENTER, 0, 0);

    notice = lv_label_create(body);
    lv_obj_set_width(notice, lv_pct(100));
    lv_obj_set_style_text_font(notice, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_align(notice, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(notice, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_update_layout(screen);
    calculator_display_init();
}
