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

#include <zmk/display/status_screen.h>

#define ZMK_DISPLAY_NAME CONFIG_LVGL_DISPLAY_DEV_NAME

static struct device *display;

static lv_obj_t *screen;

__attribute__((weak)) lv_obj_t *zmk_display_status_screen() { return NULL; }

int zmk_display_init() {
    LOG_DBG("");

    display = device_get_binding(ZMK_DISPLAY_NAME);
    if (display == NULL) {
        LOG_ERR("Failed to find display device");
        return -EINVAL;
    }

    screen = zmk_display_status_screen();

    if (screen == NULL) {
        LOG_ERR("No status screen provided");
        return 0;
    }

    lv_scr_load(screen);

    lv_task_handler();
    display_blanking_off(display);

    LOG_DBG("");
    return 0;
}

void zmk_display_task_handler() {
    lv_tick_inc(10);
    lv_task_handler();
    k_sleep(K_MSEC(10));
}
