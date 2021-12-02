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

#define PHANDLE_TO_DEVICE(node_id, prop, idx) \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define PHANDLE_TO_CHAIN_LENGTH(node_id, prop, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, chain_length),

#define PHANDLE_TO_PIXEL(node_id, prop, idx)                                      \
	{                                                                             \
		.animation = DEVICE_DT_GET(DT_PHA_BY_IDX(node_id, prop, idx, animation)), \
		.position = {                                                             \
			.x = DT_PHA_BY_IDX(node_id, prop, idx, position_x),                   \
			.y = DT_PHA_BY_IDX(node_id, prop, idx, position_y),                   \
		},                                                                        \
	},

/**
 * LED Driver device pointers.
 */
static const struct device *drivers[] = {
	DT_INST_FOREACH_PROP_ELEM(0, drivers, PHANDLE_TO_DEVICE)
};

/**
 * Size of the LED driver device pointers array.
 */
static const size_t drivers_size = DT_INST_PROP_LEN(0, drivers);

/**
 * Array containing the number of LEDs handled by each device.
 */
static const uint8_t pixels_per_driver[] = {
	DT_INST_FOREACH_PROP_ELEM(0, drivers, PHANDLE_TO_CHAIN_LENGTH)
};

/**
 * Pointers to all active animation devices.
 */
static const struct device *animations[] = {
	DT_INST_FOREACH_PROP_ELEM(0, animations, PHANDLE_TO_DEVICE)
};

/**
 * Size of the animation device pointers array.
 */
static const size_t animations_size = DT_INST_PROP_LEN(0, animations);

/**
 * Pixel configuration.
 */
static const struct animation_pixel pixels[] = {
	DT_INST_FOREACH_PROP_ELEM(0, animations, PHANDLE_TO_PIXEL)
};

/**
 * Size of the pixels array.
 */
const size_t pixels_size = DT_INST_PROP_LEN(0, pixels);

/**
 * Buffer for RGB values ready to be sent to the drivers.
 */
struct led_rgb px_buffer[DT_INST_PROP_LEN(0, pixels)];

void zmk_animation_tick() {
	for (size_t i = 0; i < ZMK_ANIMATIONS_LENGTH; ++i) {
		animation_prep_next_frame(animations[i]);
	}

	for (size_t i = 0; i < pixels_size; ++i) {
		zmk_color_rgb rgb = {
			.r = 0,
			.g = 0,
			.b = 0,
		};

		animation_get_pixel(pixel.animation, &pixel.position, &rgb);

		zmk_rgb_to_led_rgb(&rgb, &px_buffer[i]);
	}

	size_t pixels_updated = 0;

	for (size_t i = 0; i < drivers_size; ++i) {
		led_strip_update_rgb(
			drivers[i],
			&px_buffer[pixels_updated],
			pixels_per_driver[i]
		);

		pixels_updated += pixels_per_driver;
	}
}

// Set up timer here

K_WORK_DEFINE(animation_work, zmk_animation_tick);

static void zmk_animation_tick_handler(struct k_timer *timer) {
	k_work_submit(&animation_work);
}

K_TIMER_DEFINE(animation_tick, zmk_animation_tick_handler, NULL);

// Init

void zmk_animation_init(const struct device *dev) {
	// default FPS: 24, 30 or 60?
	k_timer_start(&animation_tick, K_NO_WAIT, K_MSEC(1000 / ZMK_ANIMATION_FPS));
}

// Actually, all of this might be a little hard to represent inside a config file.
// We need:
//
// - Each animation has its own node
//
// - animations[] = phandle array of animations
//
// - pixel.pos        = x,y coordinates of each pixels, could be a byte array
//
// - pixel.animations     = Number array, but it's per pixel and it's not fixed length per pixel!! PROBLEM!!!
// - pixel.blending_modes = Could be a number array again. Use some macros.
//
// - unless we say there can only be 32 different animations and use uint32_t as a bitmask?
// - but then blending modes suck and you can't order them
//

// ALTERNATIVE:
//
// Eventually we don't want to be using the device tree to set these up anyway.
// It should be possible to instantiate animations dynamically and the settings should be stored in flash.
// Maybe it's something to look into now?
//

// ALTERNATIVE no2:
//
// Look at behaviors.
// It seems that pixels themselves could be 'drivers' ?
//
// That's probably best tbh

// NOTE WHEN DEFINING PIXELS:
// See if it's maybe possible to re-use default_transform to assign pixels to keys?
// Assuming the first batch is always the keys which is not an easy assumption to make.
// Otherwise need it's own map
