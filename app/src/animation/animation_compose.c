/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_animation_compose

#include <zephyr.h>
#include <device.h>
#include <drivers/animation.h>
#include <logging/log.h>

#include <zmk/animation.h>
#include <dt-bindings/zmk/animation_compose.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Zephyr 2.7.0 comes with DT_INST_FOREACH_PROP_ELEM
// that we can't use quite yet as we're still on 2.5.*
#define ZMK_DT_INST_FOREACH_PROP_ELEM(inst, prop, fn)                                              \
    UTIL_LISTIFY(DT_INST_PROP_LEN(inst, prop), fn, DT_DRV_INST(inst), prop)

#define PHANDLE_TO_DEVICE(idx, node_id, prop) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

struct animation_compose_config {
    const struct device **animations;
    const size_t animations_size;
    const uint8_t *blending_modes;
};

static void animation_compose_render_pixel(const struct device *dev,
                                           const struct animation_pixel *pixel,
                                           struct zmk_color_rgb *value) {
    const struct animation_compose_config *config = dev->config;

    const struct device **animations = config->animations;
    const uint8_t *blending_modes = config->blending_modes;

    struct zmk_color_rgb rgb = {
        .r = 0,
        .g = 0,
        .b = 0,
    };

    for (size_t i = 0; i < config->animations_size; ++i) {
        animation_render_pixel(animations[i], pixel,
                               blending_modes[i] == BLENDING_MODE_NORMAL ? value : &rgb);

        switch (blending_modes[i]) {
        case BLENDING_MODE_MULTIPLY:
            value->r = value->r * rgb.r;
            value->g = value->g * rgb.g;
            value->b = value->b * rgb.b;
            break;
        case BLENDING_MODE_LIGHTEN:
            value->r = value->r > rgb.r ? value->r : rgb.r;
            value->g = value->g > rgb.g ? value->g : rgb.g;
            value->b = value->b > rgb.b ? value->b : rgb.b;
            break;
        case BLENDING_MODE_DARKEN:
            value->r = value->r > rgb.r ? rgb.r : value->r;
            value->g = value->g > rgb.g ? rgb.g : value->g;
            value->b = value->b > rgb.b ? rgb.b : value->b;
            break;
        case BLENDING_MODE_SCREEN:
            value->r = value->r + (1.0f - value->r) * rgb.r;
            value->g = value->g + (1.0f - value->g) * rgb.g;
            value->b = value->b + (1.0f - value->b) * rgb.b;
            break;
        case BLENDING_MODE_SUBTRACT:
            value->r = value->r - value->r * rgb.r;
            value->g = value->g - value->g * rgb.g;
            value->b = value->b - value->b * rgb.b;
            break;
        }
    }
}

static int animation_compose_init(const struct device *dev) { return 0; }

static const struct animation_api animation_compose_api = {
    .on_before_frame = NULL,
    .on_after_frame = NULL,
    .render_pixel = animation_compose_render_pixel,
};

#define ANIMATION_COMPOSE_DEVICE(idx)                                                              \
                                                                                                   \
    static const uint8_t animation_compose_##idx##_blending_modes[] =                              \
        DT_INST_PROP(idx, blending_modes);                                                         \
                                                                                                   \
    static const struct device *animation_compose_##idx##_animations[] = {                         \
        ZMK_DT_INST_FOREACH_PROP_ELEM(idx, animations, PHANDLE_TO_DEVICE)};                        \
                                                                                                   \
    static struct animation_compose_config animation_compose_##idx##_config = {                    \
        .animations = animation_compose_##idx##_animations,                                        \
        .animations_size = DT_INST_PROP_LEN(idx, animations),                                      \
        .blending_modes = animation_compose_##idx##_blending_modes,                                \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &animation_compose_init, NULL, NULL,                                \
                          &animation_compose_##idx##_config, POST_KERNEL,                          \
                          CONFIG_APPLICATION_INIT_PRIORITY, &animation_compose_api);

DT_INST_FOREACH_STATUS_OKAY(ANIMATION_COMPOSE_DEVICE);
