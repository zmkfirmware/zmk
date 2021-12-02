/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_colors_animation

#include <zephyr.h>

struct solid_color_animation_config config {
    zmk_color_hsl[] colors;
    uint8_t num_colors;
    uint16_t duration;
    uint16_t transition_duration;
};

struct solid_color_animation_data data {
    bool has_changed;

    uint16_t counter;

    zmk_color_hsl current_hsl;
    zmk_color_rgb current_rgb;
};

// or just colors array ? And if just one it stays the same otherwise it just chagnes color. genius

static void colors_animation_prep_next_frame(const struct device *dev) {
    const struct solid_color_animation_config *config = dev->config;
    const struct solid_color_animation_data *data = dev->data;

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
    zmk_hsl_to_rgb(data->current_rgb, data->current_hsl);

    data->counter = (data->counter + 1) % config.duration;
}

static void colors_animation_solid(const struct device *dev, animation_pixel *pixel) {
    const struct solid_color_animation_config config = dev->config;

    pixel->rgb = data->current_rgb;
}

static void animation_solid_color_init(const struct device *dev) {
    const struct solid_color_animation_config config = dev->config;

    // Actually... I'm not sure fi there's anything to set up here, is there?
    // But I do need a public function for has_changed
}

// #define ANIMATION_SOLID_COLOR_DEVICE(idx)
// 


// use union type to cast from uint_32t to struct { uint16_t, uint8_t, uint8_t } ?

static const struct animation_api solid_color_animation_api = {
    .prep_next_frame = solid_color_animation_prep_next_frame,
    .get_pixel = solid_color_animation_get_pixel,
};

static struct solid_color_animation_data solid_color_animation_##idx##_data;

static uint32_t solid_color_animation_##idx##_colors[DT_INST_PROP_LEN(idx, colors)] = DT_INST_PROP(idx, colors);

static struct solid_color_animation_config solid_color_animation_##idx##_config = {
    .colors = (struct zmk_color_hsl *) solid_color_animation_##idx##_colors,
    .num_colors = DT_INST_PROP_LEN(idx, colors),
    .duration = DT_INST_PROP(idx, duration),
    .transition_duration = DT_INST_PROP(idx, duration) / DT_INST_PROP_LEN(idx, colors),
};



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
