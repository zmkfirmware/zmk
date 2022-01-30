/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_animation_control

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/animation.h>
#include <logging/log.h>
#include <settings/settings.h>

#include <zmk/animation.h>
#include <zmk/animation/animation_control.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Zephyr 2.7.0 comes with DT_INST_FOREACH_PROP_ELEM
// that we can't use quite yet as we're still on 2.5.*
#define ZMK_DT_INST_FOREACH_PROP_ELEM(inst, prop, fn)                                              \
    UTIL_LISTIFY(DT_INST_PROP_LEN(inst, prop), fn, DT_DRV_INST(inst), prop)

#define PHANDLE_TO_DEVICE(idx, node_id, prop) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

struct animation_control_work_context {
    const struct device *animation;
    struct k_delayed_work save_work;
};

struct animation_control_config {
    const struct device **animations;
    const size_t animations_size;
    const uint8_t brightness_steps;
    struct animation_control_work_context *work;
    struct settings_handler *settings_handler;
};

struct animation_control_data {
    bool active;
    uint8_t brightness;
    size_t current_animation;
};

static int animation_control_load_settings(const struct device *dev, const char *name, size_t len,
                                           settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(struct animation_control_data)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, dev->data, sizeof(struct animation_control_data));
        if (rc >= 0) {
            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

static void animation_control_save_work(struct k_work *work) {
    struct animation_control_work_context *ctx =
        CONTAINER_OF(work, struct animation_control_work_context, save_work);
    const struct device *dev = ctx->animation;

    char path[40];
    snprintf(path, 40, "%s/state", dev->name);

    settings_save_one(path, dev->data, sizeof(struct animation_control_data));
};

static int animation_control_save_settings(const struct device *dev) {
    const struct animation_control_config *config = dev->config;
    struct animation_control_work_context *ctx = config->work;

    k_delayed_work_cancel(&ctx->save_work);

    return k_delayed_work_submit(&ctx->save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
}

int animation_control_handle_command(const struct device *dev, uint8_t command, uint8_t param) {
    const struct animation_control_config *config = dev->config;
    struct animation_control_data *data = dev->data;

    switch (command) {
    case ANIMATION_CMD_TOGGLE:
        data->active = !data->active;

        if (data->active) {
            animation_start(config->animations[data->current_animation]);
            break;
        }

        animation_stop(config->animations[data->current_animation]);
        break;
    case ANIMATION_CMD_NEXT:
        data->current_animation++;

        if (data->current_animation == config->animations_size) {
            data->current_animation = 0;
        }
        break;
    case ANIMATION_CMD_PREVIOUS:
        if (data->current_animation == 0) {
            data->current_animation = config->animations_size;
        }

        data->current_animation--;
        break;
    case ANIMATION_CMD_SELECT:
        if (config->animations_size < param) {
            return -ENOTSUP;
        }

        data->current_animation = param;
        break;
    case ANIMATION_CMD_DIM:
        if (data->brightness == 0) {
            return 0;
        }

        data->brightness--;

        if (data->brightness == 0) {
            animation_stop(config->animations[data->current_animation]);
        }
        break;
    case ANIMATION_CMD_BRIGHTEN:
        if (data->brightness == config->brightness_steps) {
            return 0;
        }

        if (data->brightness == 0) {
            animation_start(config->animations[data->current_animation]);
        }

        data->brightness++;
        break;
    }

    // Save the new settings
    animation_control_save_settings(dev);

    // Force refresh
    zmk_animation_request_frames(1);

    return 0;
}

static void animation_control_render_frame(const struct device *dev, struct animation_pixel *pixels,
                                           size_t num_pixels) {
    const struct animation_control_config *config = dev->config;
    const struct animation_control_data *data = dev->data;

    if (!data->active) {
        return;
    }

    animation_render_frame(config->animations[data->current_animation], pixels, num_pixels);

    if (data->brightness == config->brightness_steps) {
        return;
    }

    float brightness = (float)data->brightness / (float)config->brightness_steps;

    for (size_t i = 0; i < num_pixels; ++i) {
        pixels[i].value.r *= brightness;
        pixels[i].value.g *= brightness;
        pixels[i].value.b *= brightness;
    }
}

static void animation_control_start(const struct device *dev) {
    const struct animation_control_config *config = dev->config;
    const struct animation_control_data *data = dev->data;

    if (!data->active) {
        return;
    }

    animation_start(config->animations[data->current_animation]);
}

static void animation_control_stop(const struct device *dev) {
    const struct animation_control_config *config = dev->config;
    const struct animation_control_data *data = dev->data;

    animation_stop(config->animations[data->current_animation]);
}

static int animation_control_init(const struct device *dev) {
    const struct animation_control_config *config = dev->config;

    settings_subsys_init();

    settings_register(config->settings_handler);

    k_delayed_work_init(&config->work->save_work, animation_control_save_work);

    settings_load_subtree(dev->name);

    return 0;
}

static const struct animation_api animation_control_api = {
    .on_start = animation_control_start,
    .on_stop = animation_control_stop,
    .render_frame = animation_control_render_frame,
};

#define ANIMATION_CONTROL_DEVICE(idx)                                                              \
                                                                                                   \
    static const struct device *animation_control_##idx##_animations[] = {                         \
        ZMK_DT_INST_FOREACH_PROP_ELEM(idx, animations, PHANDLE_TO_DEVICE)};                        \
                                                                                                   \
    static struct animation_control_work_context animation_control_##idx##_work = {                \
        .animation = DEVICE_DT_GET(DT_DRV_INST(idx)),                                              \
    };                                                                                             \
                                                                                                   \
    static int animation_control_##idx##_load_settings(const char *name, size_t len,               \
                                                       settings_read_cb read_cb, void *cb_arg) {   \
        const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(idx));                                \
                                                                                                   \
        return animation_control_load_settings(dev, name, len, read_cb, cb_arg);                   \
    }                                                                                              \
                                                                                                   \
    static struct settings_handler animation_control_##idx##_settings_handler = {                  \
        .name = DT_INST_PROP(idx, label),                                                          \
        .h_set = animation_control_##idx##_load_settings,                                          \
    };                                                                                             \
                                                                                                   \
    static const struct animation_control_config animation_control_##idx##_config = {              \
        .animations = animation_control_##idx##_animations,                                        \
        .animations_size = DT_INST_PROP_LEN(idx, animations),                                      \
        .brightness_steps = DT_INST_PROP(idx, brightness_steps) - 1,                               \
        .work = &animation_control_##idx##_work,                                                   \
        .settings_handler = &animation_control_##idx##_settings_handler,                           \
    };                                                                                             \
                                                                                                   \
    static struct animation_control_data animation_control_##idx##_data = {                        \
        .active = true,                                                                            \
        .brightness = DT_INST_PROP(idx, brightness_steps) - 1,                                     \
        .current_animation = 0,                                                                    \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(idx, &animation_control_init, NULL, &animation_control_##idx##_data,     \
                          &animation_control_##idx##_config, POST_KERNEL,                          \
                          CONFIG_APPLICATION_INIT_PRIORITY, &animation_control_api);

DT_INST_FOREACH_STATUS_OKAY(ANIMATION_CONTROL_DEVICE);
