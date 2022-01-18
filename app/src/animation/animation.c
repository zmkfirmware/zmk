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

#include <stdlib.h>
#include <math.h>

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
        .position_x = DT_PHA_BY_IDX(node_id, prop, idx, position_x),                               \
        .position_y = DT_PHA_BY_IDX(node_id, prop, idx, position_y),                               \
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
 * Pointer to the root animation
 */
static const struct device *animation_root = DEVICE_DT_GET(DT_CHOSEN(zmk_animation));

/**
 * Pixel configuration.
 */
static struct animation_pixel pixels[] = {
    ZMK_DT_INST_FOREACH_PROP_ELEM(0, pixels, PHANDLE_TO_PIXEL)};

/**
 * Size of the pixels array.
 */
static const size_t pixels_size = DT_INST_PROP_LEN(0, pixels);

/**
 * Buffer for RGB values ready to be sent to the drivers.
 */
static struct led_rgb px_buffer[DT_INST_PROP_LEN(0, pixels)];

/**
 * Counter for animation frames that have been requested but have yet to be executed.
 */
static uint32_t animation_timer_countdown = 0;

/**
 * Conditional implementation of zmk_animation_get_pixel_by_key_position
 * if key-pixels is set.
 */
#if DT_INST_NODE_HAS_PROP(0, key_position)
static const uint8_t pixels_by_key_position[] = DT_INST_PROP(0, key_pixels);

size_t zmk_animation_get_pixel_by_key_position(size_t key_position) {
    return pixels_by_key_position[key_position];
}
#endif

#if defined(CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE) && (CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE == 1)

/**
 * Lookup table for distance between any two pixels.
 *
 * The values are stored as a triangular matrix which cuts the space requirement roughly in half.
 */
static uint8_t
    pixel_distance[((DT_INST_PROP_LEN(0, pixels) + 1) * DT_INST_PROP_LEN(0, pixels)) / 2];

uint8_t zmk_animation_get_pixel_distance(size_t pixel_idx, size_t other_pixel_idx) {
    if (pixel_idx < other_pixel_idx) {
        return zmk_animation_get_pixel_distance(other_pixel_idx, pixel_idx);
    }

    return pixel_distance[(((pixel_idx + 1) * pixel_idx) >> 1) + other_pixel_idx];
}

#endif

static void zmk_animation_tick(struct k_work *work) {
    LOG_DBG("Animation tick");
    animation_render_frame(animation_root, &pixels[0], pixels_size);

    for (size_t i = 0; i < pixels_size; ++i) {
        zmk_rgb_to_led_rgb(&pixels[i].value, &px_buffer[i]);

        // Reset values for the next cycle
        pixels[i].value.r = 0;
        pixels[i].value.g = 0;
        pixels[i].value.b = 0;
    }

    size_t pixels_updated = 0;

    for (size_t i = 0; i < drivers_size; ++i) {
        led_strip_update_rgb(drivers[i], &px_buffer[pixels_updated], pixels_per_driver[i]);

        pixels_updated += (size_t)pixels_per_driver;
    }
}

K_WORK_DEFINE(animation_work, zmk_animation_tick);

static void zmk_animation_tick_handler(struct k_timer *timer) {
    if (--animation_timer_countdown == 0) {
        k_timer_stop(timer);
    }

    k_work_submit(&animation_work);
}

K_TIMER_DEFINE(animation_tick, zmk_animation_tick_handler, NULL);

void zmk_animation_request_frames(uint32_t frames) {
    if (frames <= animation_timer_countdown) {
        return;
    }

    if (animation_timer_countdown == 0) {
        k_timer_start(&animation_tick, K_MSEC(1000 / CONFIG_ZMK_ANIMATION_FPS),
                      K_MSEC(1000 / CONFIG_ZMK_ANIMATION_FPS));
    }

    animation_timer_countdown = frames;
}

static int zmk_animation_init(const struct device *dev) {
#if defined(CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE) && (CONFIG_ZMK_ANIMATION_PIXEL_DISTANCE == 1)
    // Prefill the pixel distance lookup table
    int k = 0;
    for (size_t i = 0; i < pixels_size; ++i) {
        for (size_t j = 0; j <= i; ++j) {
            // Distances are normalized to fit inside 0-255 range to fit inside uint8_t
            // for better space efficiency
            pixel_distance[k++] = sqrt(pow(pixels[i].position_x - pixels[j].position_x, 2) +
                                       pow(pixels[i].position_y - pixels[j].position_y, 2)) *
                                  255 / 360;
        }
    }
#endif

    LOG_INF("ZMK Animation Ready");

    animation_start(animation_root);

    return 0;
}

SYS_INIT(zmk_animation_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
