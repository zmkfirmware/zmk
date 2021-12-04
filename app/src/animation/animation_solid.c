/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_animation_solid

#include <zephyr.h>
#include <device.h>
#include <drivers/animation.h>
#include <logging/log.h>

#include <zmk/animation.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct animation_solid_config {
    struct zmk_color_hsl *colors;
    uint8_t num_colors;
    uint16_t duration;
    uint16_t transition_duration;
};

struct animation_solid_data {
    bool has_changed;

    uint16_t counter;

    struct zmk_color_hsl current_hsl;
    struct zmk_color_rgb current_rgb;
};

static void animation_solid_prep_next_frame(const struct device *dev) {
    const struct animation_solid_config *config = dev->config;
    struct animation_solid_data *data = dev->data;

    // Animation only contains a single color, nothing to do
    if (config->num_colors == 1) {
        return;
    }

    const size_t from = data->counter / config->transition_duration;
    const size_t to = (from + 1) % config->num_colors;

    struct zmk_color_hsl next_hsl;

    zmk_interpolate_hsl(
        &config->colors[from],
        &config->colors[to],
        &next_hsl,
        (data->counter % config->transition_duration) / (float) config->transition_duration
    );

    data->has_changed = !zmk_cmp_hsl(&data->current_hsl, &next_hsl);

    data->current_hsl = next_hsl;
    zmk_hsl_to_rgb(&data->current_hsl, &data->current_rgb);

    data->counter = (data->counter + 1) % config->duration;
}

static void animation_solid_get_pixel(const struct device *dev, const struct animation_pixel_position *position,
                                        struct zmk_color_rgb *value) {
    const struct animation_solid_data *data = dev->data;

    value->r = data->current_rgb.r;
    value->g = data->current_rgb.g;
    value->b = data->current_rgb.b;
}

static int animation_solid_init(const struct device *dev) {
    const struct animation_solid_config *config = dev->config;
    struct animation_solid_data *data = dev->data;

    data->counter = 0;
    data->current_hsl = config->colors[0];

    zmk_hsl_to_rgb(&data->current_hsl, &data->current_rgb);

    return 0;
}

static const struct animation_api animation_solid_api = {
    .prep_next_frame = animation_solid_prep_next_frame,
    .get_pixel = animation_solid_get_pixel,
};

#define ANIMATION_SOLID_DEVICE(idx)                                                                                      \
                                                                                                                         \
    static struct animation_solid_data animation_solid_##idx##_data;                                                     \
                                                                                                                         \
    static uint32_t animation_solid_##idx##_colors[DT_INST_PROP_LEN(idx, colors)] = DT_INST_PROP(idx, colors);           \
                                                                                                                         \
    static struct animation_solid_config animation_solid_##idx##_config = {                                              \
        .colors = (struct zmk_color_hsl *) animation_solid_##idx##_colors,                                               \
        .num_colors = DT_INST_PROP_LEN(idx, colors),                                                                     \
        .duration = DT_INST_PROP(idx, duration) * CONFIG_ZMK_ANIMATION_FPS,                                              \
        .transition_duration = (DT_INST_PROP(idx, duration) * CONFIG_ZMK_ANIMATION_FPS) / DT_INST_PROP_LEN(idx, colors), \
    };                                                                                                                   \
                                                                                                                         \
    DEVICE_DT_INST_DEFINE(idx, &animation_solid_init, NULL, &animation_solid_##idx##_data,                               \
                            &animation_solid_##idx##_config, POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,                \
                            &animation_solid_api);                                                                                                                         

DT_INST_FOREACH_STATUS_OKAY(ANIMATION_SOLID_DEVICE);


// To do:
//
// STEP 1: single animation
// - Start with a single animation, just color
// - Add layer for taking the output from here and putting it to the led strip
// - Make it work
//
// STEP 2: areas, in fact, instead of defining them explicitly we can just use appropriate x,y coordinates and animation.
// - Split keyboard in two independent areas
// - Make it work
//
// STEP 3: add additional animation effects
// - Basically, carry over rgb_underglow.
// - Make it work
//
// STEP 4: add animation triggers
// - Allow an animation to be triggered by behaviors or key-presses
// - Make it work
//
// STEP 5: add animation layers and a MULTIPLY mode (again, opacity would be set on individual pixels so... that affects some optimizations I guess)
// - Normal mode: overrides layers below
// - Multiply mode: auguments whatever is below (opacity, whatever)
//
// Voila! Animation composition!
//
// STEP 6, BONUS!:
// - Figure out a way to switch animations during runtime?
//
// Notes:
// - Any animation settings go into 'driver' config & data, so they can be updated at runtime.
// - Main limitation is space, so the amount of different animations one can have loaded
//
// More notes:
// - Solid color would be one animation (just transitions between colors)
// - Gradient (SPECTRUM) would be another, you choose how they're distributed accross the keys and if they move?
// - Effects like 'breathe' can be implemented by specifying #000 as one of the colors or using a multiply layer?
