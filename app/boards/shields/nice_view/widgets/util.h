/*
 *
 * Copyright (c) 2025 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <lvgl.h>
#include <zmk/endpoints.h>

#define NICEVIEW_PROFILE_COUNT 5

#define CANVAS_SIZE 68
#define CANVAS_COLOR_FORMAT LV_COLOR_FORMAT_L8 // smallest type supported by sw_rotate
#define CANVAS_BUF_SIZE                                                                            \
    LV_CANVAS_BUF_SIZE(CANVAS_SIZE, CANVAS_SIZE, LV_COLOR_FORMAT_GET_BPP(CANVAS_COLOR_FORMAT),     \
                       LV_DRAW_BUF_STRIDE_ALIGN)

#define LVGL_BACKGROUND                                                                            \
    IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black() : lv_color_white()
#define LVGL_FOREGROUND                                                                            \
    IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white() : lv_color_black()

struct status_state {
    uint8_t battery;
    bool charging;
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool profiles_connected[NICEVIEW_PROFILE_COUNT];
    bool profiles_bonded[NICEVIEW_PROFILE_COUNT];
    uint8_t layer_index;
    const char *layer_label;
    uint8_t wpm[10];
#else
    bool connected;
#endif
};

struct battery_status_state {
    uint8_t level;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_present;
#endif
};

void rotate_canvas(lv_obj_t *canvas);
void draw_battery(lv_obj_t *canvas, const struct status_state *state);
void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);
void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width);
void init_arc_dsc(lv_draw_arc_dsc_t *arc_dsc, lv_color_t color, uint8_t width);

void canvas_draw_line(lv_obj_t *canvas, const lv_point_t points[], uint32_t point_cnt,
                      lv_draw_line_dsc_t *draw_dsc);
void canvas_draw_rect(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                      lv_draw_rect_dsc_t *draw_dsc);
void canvas_draw_arc(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t r,
                     int32_t start_angle, int32_t end_angle, lv_draw_arc_dsc_t *draw_dsc);
void canvas_draw_text(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t max_w,
                      lv_draw_label_dsc_t *draw_dsc, const char *txt);
void canvas_draw_img(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, const lv_image_dsc_t *src,
                     lv_draw_image_dsc_t *draw_dsc);
