/*
 * Copyright (c) 2022 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_strip_composite

#include <drivers/led_strip.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(led_strip_composite);

#include <zephyr.h>
#include <device.h>

struct led_strip_child {
    const struct device *device;
    uint32_t length;
};

struct led_strip_composite_config {
    const struct led_strip_child *strips;
    uint32_t strips_cnt;
    uint32_t pixels_cnt;
};

static int led_strip_composite_update_rgb(const struct device *dev, struct led_rgb *pixels,
                                          size_t num_pixels) {
    const struct led_strip_composite_config *config = dev->config;

    if (num_pixels > config->pixels_cnt) {
        num_pixels = config->pixels_cnt;
    }

    int ret = 0;
    uint32_t offset = 0, length = 0;
    for (int i = 0; i < config->strips_cnt && offset < num_pixels; i++) {
        length = config->strips[i].length;
        if (length > num_pixels - offset) {
            length = num_pixels - offset;
        }

        LOG_DBG("Updating led_strip %d, offset: %d, length: %d", i, offset, length);

        ret = led_strip_update_rgb(config->strips[i].device, &pixels[offset], length);
        if (ret != 0) {
            LOG_ERR("Failed updating child led_strip device %s", config->strips[i].device->name);
            return ret;
        }

        offset += length;
    }

    return 0;
}

static int led_strip_composite_update_channels(const struct device *dev, uint8_t *channels,
                                               size_t num_channels) {
    LOG_ERR("update_channels not implemented");
    return -ENOTSUP;
}

static int led_strip_composite_init(const struct device *dev) {
    const struct led_strip_composite_config *config = dev->config;

    for (int i = 0; i < config->strips_cnt; i++) {
        LOG_INF("Bond led_strip %d: %s, length: %d", i, config->strips[i].device->name,
                config->strips[i].length);
    }

    return 0;
}

static const struct led_strip_driver_api led_strip_composite_api = {
    .update_rgb = led_strip_composite_update_rgb,
    .update_channels = led_strip_composite_update_channels,
};

#define LED_STRIP_LENGTH(inst) DT_PROP(DT_PHANDLE(inst, led_strip), chain_length)

#define LED_STRIP_CHILD(inst)                                                                      \
    {                                                                                              \
        .device = DEVICE_DT_GET(DT_PHANDLE(inst, led_strip)),                                      \
        .length = LED_STRIP_LENGTH(inst),                                                          \
    },

static const struct led_strip_child led_strip_children[] = {
    DT_INST_FOREACH_CHILD(0, LED_STRIP_CHILD)};

static const struct led_strip_composite_config led_strip_composite_config = {
    .strips = led_strip_children,
    .strips_cnt = ARRAY_SIZE(led_strip_children),
    .pixels_cnt = DT_INST_PROP(0, chain_length),
};

DEVICE_DT_INST_DEFINE(0, &led_strip_composite_init, NULL, NULL, &led_strip_composite_config,
                      POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY, &led_strip_composite_api);
