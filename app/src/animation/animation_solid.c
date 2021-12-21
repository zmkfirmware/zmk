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
    size_t *pixel_map;
    size_t pixel_map_size;
    struct zmk_color_hsl *colors;
    uint8_t num_colors;
    uint16_t duration;
    uint16_t transition_duration;
};

struct animation_solid_data {
    uint16_t counter;

    struct zmk_color_hsl current_hsl;
    struct zmk_color_rgb current_rgb;
};

static void animation_solid_update_color(const struct device *dev) {
    const struct animation_solid_config *config = dev->config;
    struct animation_solid_data *data = dev->data;

    const size_t from = data->counter / config->transition_duration;
    const size_t to = (from + 1) % config->num_colors;

    struct zmk_color_hsl next_hsl;

    zmk_interpolate_hsl(&config->colors[from], &config->colors[to], &next_hsl,
                        (data->counter % config->transition_duration) /
                            (float)config->transition_duration);

    data->current_hsl = next_hsl;
    zmk_hsl_to_rgb(&data->current_hsl, &data->current_rgb);

    data->counter = (data->counter + 1) % config->duration;
}

static void animation_solid_render_frame(const struct device *dev, struct animation_pixel *pixels,
                                         size_t num_pixels) {
    const struct animation_solid_config *config = dev->config;
    struct animation_solid_data *data = dev->data;

    for (size_t i = 0; i < config->pixel_map_size; ++i) {
        pixels[config->pixel_map[i]].value = data->current_rgb;
    }

    if (config->num_colors == 1) {
        return;
    }

    // Request frames on counter reset
    if (data->counter == 0) {
        zmk_animation_request_frames(config->duration);
    }

    animation_solid_update_color(dev);
}

static void animation_solid_start(const struct device *dev) { zmk_animation_request_frames(1); }

static void animation_solid_stop(const struct device *dev) {
    // Nothing to do.
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
    .on_start = animation_solid_start,
    .on_stop = animation_solid_stop,
    .render_frame = animation_solid_render_frame,
};

#define ANIMATION_SOLID_DEVICE(idx)                                                                \
                                                                                                   \
    static struct animation_solid_data animation_solid_##idx##_data;                               \
                                                                                                   \
    static size_t animation_ripple_##idx##_pixel_map[] = DT_INST_PROP(idx, pixels);                \
                                                                                                   \
    static uint32_t animation_solid_##idx##_colors[] = DT_INST_PROP(idx, colors);                  \
                                                                                                   \
    static struct animation_solid_config animation_solid_##idx##_config = {                        \
        .pixel_map = &animation_ripple_##idx##_pixel_map[0],                                       \
        .pixel_map_size = DT_INST_PROP_LEN(idx, pixels),                                           \
        .colors = (struct zmk_color_hsl *)animation_solid_##idx##_colors,                          \
        .num_colors = DT_INST_PROP_LEN(idx, colors),                                               \
        .duration = DT_INST_PROP(idx, duration) * CONFIG_ZMK_ANIMATION_FPS,                        \
        .transition_duration = (DT_INST_PROP(idx, duration) * CONFIG_ZMK_ANIMATION_FPS) /          \
                               DT_INST_PROP_LEN(idx, colors),                                      \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &animation_solid_init, NULL, &animation_solid_##idx##_data,         \
                          &animation_solid_##idx##_config, POST_KERNEL,                            \
                          CONFIG_APPLICATION_INIT_PRIORITY, &animation_solid_api);

DT_INST_FOREACH_STATUS_OKAY(ANIMATION_SOLID_DEVICE);
