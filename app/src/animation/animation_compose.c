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

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Zephyr 2.7.0 comes with DT_INST_FOREACH_PROP_ELEM
// that we can't use quite yet as we're still on 2.5.*
#define ZMK_DT_INST_FOREACH_PROP_ELEM(inst, prop, fn)                                              \
    UTIL_LISTIFY(DT_INST_PROP_LEN(inst, prop), fn, DT_DRV_INST(inst), prop)

#define PHANDLE_TO_DEVICE(idx, node_id, prop) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

struct animation_compose_config {
    const struct device **animations;
    const size_t animations_size;
};

static void animation_compose_render_frame(const struct device *dev, struct animation_pixel *pixels,
                                           size_t num_pixels) {
    const struct animation_compose_config *config = dev->config;

    for (size_t i = 0; i < config->animations_size; ++i) {
        animation_render_frame(config->animations[i], pixels, num_pixels);
    }
}

static int animation_compose_init(const struct device *dev) { return 0; }

static const struct animation_api animation_compose_api = {
    .render_frame = animation_compose_render_frame,
};

#define ANIMATION_COMPOSE_DEVICE(idx)                                                              \
                                                                                                   \
    static const struct device *animation_compose_##idx##_animations[] = {                         \
        ZMK_DT_INST_FOREACH_PROP_ELEM(idx, animations, PHANDLE_TO_DEVICE)};                        \
                                                                                                   \
    static struct animation_compose_config animation_compose_##idx##_config = {                    \
        .animations = animation_compose_##idx##_animations,                                        \
        .animations_size = DT_INST_PROP_LEN(idx, animations),                                      \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &animation_compose_init, NULL, NULL,                                \
                          &animation_compose_##idx##_config, POST_KERNEL,                          \
                          CONFIG_APPLICATION_INIT_PRIORITY, &animation_compose_api);

DT_INST_FOREACH_STATUS_OKAY(ANIMATION_COMPOSE_DEVICE);
