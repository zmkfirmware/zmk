/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <init.h>
#include <device.h>
#include <devicetree.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/display.h>
#include <lvgl.h>

#define ZMK_DISPLAY_NAME CONFIG_LVGL_DISPLAY_DEV_NAME

static struct device *display;

static lv_obj_t *screen;

int zmk_display_init() {
    lv_obj_t *hello_world_label;
    lv_obj_t *count_label;

    LOG_DBG("");

    display = device_get_binding(ZMK_DISPLAY_NAME);
    if (display == NULL) {
        LOG_ERR("Failed to find display device");
        return -EINVAL;
    }

    screen = lv_obj_create(NULL, NULL);
    lv_scr_load(screen);

    hello_world_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(hello_world_label, "ZMK v0.1.0");
    lv_obj_align(hello_world_label, NULL, LV_ALIGN_CENTER, 0, 0);
    count_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(count_label, CONFIG_ZMK_KEYBOARD_NAME);
    lv_obj_align(count_label, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_task_handler();
    display_blanking_off(display);

    return 0;
}

void zmk_display_task_handler() {
    lv_tick_inc(10);
    lv_task_handler();
    k_sleep(K_MSEC(10));
}
