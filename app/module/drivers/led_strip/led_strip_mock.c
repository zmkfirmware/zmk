/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_strip_mock

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define LED_STRIP_MOCK_PIXEL_LEN(inst)                                                             \
    ((DT_INST_PROP(inst, chain_length) > 0) ? DT_INST_PROP(inst, chain_length) : 1)
#define LED_STRIP_MOCK_CHANNEL_LEN(inst) (LED_STRIP_MOCK_PIXEL_LEN(inst) * 3U)

struct zmk_led_strip_mock_config {
    size_t length;
    bool support_update_channels;
};

struct zmk_led_strip_mock_data {
    struct led_rgb *pixels;
    uint8_t *channels;
};

static int zmk_led_strip_mock_update_rgb(const struct device *dev, struct led_rgb *pixels,
                                         size_t num_pixels) {
    const struct zmk_led_strip_mock_config *config = dev->config;
    struct zmk_led_strip_mock_data *data = dev->data;

    if (pixels == NULL || num_pixels > config->length) {
        return -EINVAL;
    }

    for (size_t i = 0; i < num_pixels; i++) {
        if (memcmp(&data->pixels[i], &pixels[i], sizeof(struct led_rgb)) == 0) {
            continue;
        }

        data->pixels[i] = pixels[i];
        data->channels[(i * 3U) + 0U] = pixels[i].r;
        data->channels[(i * 3U) + 1U] = pixels[i].g;
        data->channels[(i * 3U) + 2U] = pixels[i].b;
        LOG_INF("LED strip mock pixel[%u]=%u,%u,%u", (uint32_t)i, pixels[i].r, pixels[i].g,
                pixels[i].b);
    }

    return 0;
}

static int zmk_led_strip_mock_update_channels(const struct device *dev, uint8_t *channels,
                                              size_t num_channels) {
    const struct zmk_led_strip_mock_config *config = dev->config;
    struct zmk_led_strip_mock_data *data = dev->data;

    if (channels == NULL || num_channels > (config->length * 3U)) {
        return -EINVAL;
    }

    for (size_t i = 0; i < num_channels; i++) {
        if (data->channels[i] == channels[i]) {
            continue;
        }

        data->channels[i] = channels[i];
        data->pixels[i / 3U].r = data->channels[(i / 3U) * 3U];
        data->pixels[i / 3U].g = data->channels[((i / 3U) * 3U) + 1U];
        data->pixels[i / 3U].b = data->channels[((i / 3U) * 3U) + 2U];
        LOG_INF("LED strip mock channel[%u]=%u", (uint32_t)i, channels[i]);
    }

    return 0;
}

static size_t zmk_led_strip_mock_length(const struct device *dev) {
    const struct zmk_led_strip_mock_config *config = dev->config;

    return config->length;
}

static int zmk_led_strip_mock_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

static DEVICE_API(led_strip, zmk_led_strip_mock_api) = {
    .update_rgb = zmk_led_strip_mock_update_rgb,
    .length = zmk_led_strip_mock_length,
};

static DEVICE_API(led_strip, zmk_led_strip_mock_channels_api) = {
    .update_rgb = zmk_led_strip_mock_update_rgb,
    .update_channels = zmk_led_strip_mock_update_channels,
    .length = zmk_led_strip_mock_length,
};

#define LED_STRIP_MOCK_API(inst)                                                                   \
    (DT_INST_PROP(inst, support_update_channels) ? &zmk_led_strip_mock_channels_api                \
                                                 : &zmk_led_strip_mock_api)

#define LED_STRIP_MOCK_INST(inst)                                                                   \
    static struct led_rgb zmk_led_strip_mock_pixels_##inst[LED_STRIP_MOCK_PIXEL_LEN(inst)];        \
    static uint8_t zmk_led_strip_mock_channels_##inst[LED_STRIP_MOCK_CHANNEL_LEN(inst)];           \
    static struct zmk_led_strip_mock_data zmk_led_strip_mock_data_##inst = {                        \
        .pixels = zmk_led_strip_mock_pixels_##inst,                                                 \
        .channels = zmk_led_strip_mock_channels_##inst,                                             \
    };                                                                                              \
    static const struct zmk_led_strip_mock_config zmk_led_strip_mock_config_##inst = {              \
        .length = DT_INST_PROP(inst, chain_length),                                                 \
        .support_update_channels = DT_INST_PROP(inst, support_update_channels),                     \
    };                                                                                              \
    DEVICE_DT_INST_DEFINE(inst, zmk_led_strip_mock_init, NULL, &zmk_led_strip_mock_data_##inst,    \
                          &zmk_led_strip_mock_config_##inst, POST_KERNEL,                           \
                          CONFIG_LED_STRIP_INIT_PRIORITY, LED_STRIP_MOCK_API(inst));

DT_INST_FOREACH_STATUS_OKAY(LED_STRIP_MOCK_INST)
