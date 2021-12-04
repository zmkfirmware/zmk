/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_animation

#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <kernel.h>

#include <logging/log.h>

#include <drivers/animation.h>
#include <drivers/led_strip.h>

#include <zmk/animation.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Zephyr 2.7.0 comes with DT_INST_FOREACH_PROP_ELEM
// that we can't use quite yet as we're still on 2.5.*
#define ZMK_DT_INST_FOREACH_PROP_ELEM(inst, prop, fn)                                              \
    UTIL_LISTIFY(DT_INST_PROP_LEN(inst, prop), fn, DT_DRV_INST(inst), prop)

#define PHANDLE_TO_DEVICE(idx, node_id, prop) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define PHANDLE_TO_CHAIN_LENGTH(idx, node_id, prop)                                                \
    DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, chain_length),

#define PHANDLE_TO_PIXEL(idx, node_id, prop)                                                       \
    {                                                                                              \
        .animation = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),                         \
        .position =                                                                                \
            {                                                                                      \
                .x = DT_PHA_BY_IDX(node_id, prop, idx, position_x),                                \
                .y = DT_PHA_BY_IDX(node_id, prop, idx, position_y),                                \
            },                                                                                     \
    },

/**
 * LED Driver device pointers.
 */
static const struct device *drivers[] = {
    ZMK_DT_INST_FOREACH_PROP_ELEM(0, drivers, PHANDLE_TO_DEVICE)};

/**
 * Size of the LED driver device pointers array.
 */
static const size_t drivers_size = DT_INST_PROP_LEN(0, drivers);

/**
 * Array containing the number of LEDs handled by each device.
 */
static const uint8_t pixels_per_driver[] = {
    ZMK_DT_INST_FOREACH_PROP_ELEM(0, drivers, PHANDLE_TO_CHAIN_LENGTH)};

/**
 * Pointers to all active animation devices.
 */
static const struct device *animations[] = {
    ZMK_DT_INST_FOREACH_PROP_ELEM(0, animations, PHANDLE_TO_DEVICE)};

/**
 * Size of the animation device pointers array.
 */
static const size_t animations_size = DT_INST_PROP_LEN(0, animations);

/**
 * Pixel configuration.
 */
static const struct animation_pixel pixels[] = {
    ZMK_DT_INST_FOREACH_PROP_ELEM(0, pixels, PHANDLE_TO_PIXEL)};

/**
 * Size of the pixels array.
 */
static const size_t pixels_size = DT_INST_PROP_LEN(0, pixels);

/**
 * Buffer for RGB values ready to be sent to the drivers.
 */
static struct led_rgb px_buffer[DT_INST_PROP_LEN(0, pixels)];

static void zmk_animation_tick(struct k_work *work) {
    for (size_t i = 0; i < animations_size; ++i) {
        animation_prep_next_frame(animations[i]);
    }

    for (size_t i = 0; i < pixels_size; ++i) {
        struct zmk_color_rgb rgb = {
            .r = 0,
            .g = 0,
            .b = 0,
        };

        animation_get_pixel(pixels[i].animation, &pixels[i].position, &rgb);

        zmk_rgb_to_led_rgb(&rgb, &px_buffer[i]);
    }

    size_t pixels_updated = 0;

    for (size_t i = 0; i < drivers_size; ++i) {
        led_strip_update_rgb(drivers[i], &px_buffer[pixels_updated], pixels_per_driver[i]);

        pixels_updated += (size_t)pixels_per_driver;
    }
}

K_WORK_DEFINE(animation_work, zmk_animation_tick);

static void zmk_animation_tick_handler(struct k_timer *timer) { k_work_submit(&animation_work); }

K_TIMER_DEFINE(animation_tick, zmk_animation_tick_handler, NULL);

static int zmk_animation_init(const struct device *dev) {
    LOG_INF("ZMK Animation Ready");

    k_timer_start(&animation_tick, K_NO_WAIT, K_MSEC(1000 / CONFIG_ZMK_ANIMATION_FPS));

    return 0;
}

SYS_INIT(zmk_animation_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
