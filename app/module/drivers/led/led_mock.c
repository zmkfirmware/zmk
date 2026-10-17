/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_mock

#include <drivers/led/led_mock.h>

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define ZMK_LED_MOCK_MODE_BINARY 0
#define ZMK_LED_MOCK_MODE_BRIGHTNESS 1
#define ZMK_LED_MOCK_MODE_COLOR 2
#define ZMK_LED_MOCK_MODE_CHANNELS 3
#define ZMK_LED_MOCK_MODE_NONE 4

#define CHILD_COUNT(node_id) +1
#define LED_MOCK_CHILD_COUNT(inst) (0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, CHILD_COUNT))
#define LED_MOCK_CHILD_ARRAY_LEN(inst)                                                             \
    ((LED_MOCK_CHILD_COUNT(inst) > 0) ? LED_MOCK_CHILD_COUNT(inst) : 1)
#define LED_MOCK_CHANNEL_ARRAY_LEN(inst)                                                           \
    ((DT_INST_PROP_OR(inst, channel_count, 0) > 0) ? DT_INST_PROP_OR(inst, channel_count, 0) : 1)

struct zmk_led_mock_config {
    uint8_t mode;
    uint8_t managed_power_kind;
    const struct led_info *led_infos;
    uint16_t led_count;
    uint16_t channel_count;
};

struct zmk_led_mock_data {
    uint8_t *brightness_values;
    uint8_t *color_values;
    uint8_t *channel_values;
    bool blanked;
};

static int zmk_led_mock_set_binary_state(const struct device *dev, uint32_t led, bool on) {
    const struct zmk_led_mock_config *config = dev->config;
    struct zmk_led_mock_data *data = dev->data;
    uint8_t value = on ? 1U : 0U;

    if (led >= config->led_count) {
        return -EINVAL;
    }

    if (data->brightness_values[led] == value) {
        return 0;
    }

    data->brightness_values[led] = value;
    LOG_INF("LED mock binary[%u]=%s", led, on ? "on" : "off");
    return 0;
}

static int zmk_led_mock_set_brightness_value(const struct device *dev, uint32_t led,
                                             uint8_t value) {
    const struct zmk_led_mock_config *config = dev->config;
    struct zmk_led_mock_data *data = dev->data;

    if (led >= config->led_count || value > 100U) {
        return -EINVAL;
    }

    if (data->brightness_values[led] == value) {
        return 0;
    }

    data->brightness_values[led] = value;
    LOG_INF("LED mock brightness[%u]=%u", led, value);
    return 0;
}

static int zmk_led_mock_set_color_value(const struct device *dev, uint32_t led, uint8_t num_colors,
                                        const uint8_t *color) {
    const struct zmk_led_mock_config *config = dev->config;
    struct zmk_led_mock_data *data = dev->data;
    const struct led_info *info;
    uint8_t *stored;

    if (led >= config->led_count || color == NULL) {
        return -EINVAL;
    }

    info = &config->led_infos[led];
    if (num_colors != info->num_colors) {
        return -EINVAL;
    }

    stored = &data->color_values[led * 3U];
    if (memcmp(stored, color, num_colors) == 0) {
        return 0;
    }

    memcpy(stored, color, num_colors);
    LOG_INF("LED mock color[%u]=%u,%u,%u", led, stored[0], stored[1], stored[2]);
    return 0;
}

static int zmk_led_mock_write_channels_value(const struct device *dev, uint32_t start_channel,
                                             uint32_t num_channels, const uint8_t *buf) {
    const struct zmk_led_mock_config *config = dev->config;
    struct zmk_led_mock_data *data = dev->data;

    if (buf == NULL || (start_channel + num_channels) > config->channel_count) {
        return -EINVAL;
    }

    for (uint32_t i = 0; i < num_channels; i++) {
        uint32_t channel = start_channel + i;

        if (data->channel_values[channel] == buf[i]) {
            continue;
        }

        data->channel_values[channel] = buf[i];
        LOG_INF("LED mock channel[%u]=%u", channel, buf[i]);
    }

    return 0;
}

static int zmk_led_mock_get_info(const struct device *dev, uint32_t led,
                                 const struct led_info **info) {
    const struct zmk_led_mock_config *config = dev->config;

    if (led >= config->led_count || info == NULL) {
        return -EINVAL;
    }

    *info = &config->led_infos[led];
    return 0;
}

static int zmk_led_mock_on(const struct device *dev, uint32_t led) {
    const struct zmk_led_mock_config *config = dev->config;
    struct zmk_led_mock_data *data = dev->data;

    switch (config->mode) {
    case ZMK_LED_MOCK_MODE_BINARY:
        return zmk_led_mock_set_binary_state(dev, led, true);
    case ZMK_LED_MOCK_MODE_BRIGHTNESS:
        return zmk_led_mock_set_brightness_value(dev, led, 100U);
    case ZMK_LED_MOCK_MODE_COLOR: {
        uint8_t on_values[3] = {255U, 255U, 255U};
        return zmk_led_mock_set_color_value(dev, led, 3U, on_values);
    }
    case ZMK_LED_MOCK_MODE_CHANNELS:
        if (led >= config->channel_count) {
            return -EINVAL;
        }
        if (data->channel_values[led] == 255U) {
            return 0;
        }
        data->channel_values[led] = 255U;
        LOG_INF("LED mock channel[%u]=255", led);
        return 0;
    default:
        return -ENOTSUP;
    }
}

static int zmk_led_mock_off(const struct device *dev, uint32_t led) {
    const struct zmk_led_mock_config *config = dev->config;
    struct zmk_led_mock_data *data = dev->data;

    switch (config->mode) {
    case ZMK_LED_MOCK_MODE_BINARY:
        return zmk_led_mock_set_binary_state(dev, led, false);
    case ZMK_LED_MOCK_MODE_BRIGHTNESS:
        return zmk_led_mock_set_brightness_value(dev, led, 0U);
    case ZMK_LED_MOCK_MODE_COLOR: {
        uint8_t off_values[3] = {0U, 0U, 0U};
        return zmk_led_mock_set_color_value(dev, led, 3U, off_values);
    }
    case ZMK_LED_MOCK_MODE_CHANNELS:
        if (led >= config->channel_count) {
            return -EINVAL;
        }
        if (data->channel_values[led] == 0U) {
            return 0;
        }
        data->channel_values[led] = 0U;
        LOG_INF("LED mock channel[%u]=0", led);
        return 0;
    default:
        return -ENOTSUP;
    }
}

static int zmk_led_mock_set_brightness(const struct device *dev, uint32_t led, uint8_t value) {
    return zmk_led_mock_set_brightness_value(dev, led, value);
}

static int zmk_led_mock_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
                                  const uint8_t *color) {
    return zmk_led_mock_set_color_value(dev, led, num_colors, color);
}

static int zmk_led_mock_write_channels(const struct device *dev, uint32_t start_channel,
                                       uint32_t num_channels, const uint8_t *buf) {
    return zmk_led_mock_write_channels_value(dev, start_channel, num_channels, buf);
}

int zmk_led_mock_blank(const struct device *dev, bool blank_en) {
    const struct zmk_led_mock_config *config;
    struct zmk_led_mock_data *data;

    if (dev == NULL) {
        return -EINVAL;
    }

    config = dev->config;
    data = dev->data;

    if (config->managed_power_kind == ZMK_LED_MOCK_MANAGED_POWER_NONE) {
        return -ENOTSUP;
    }

    if (data->blanked == blank_en) {
        return 0;
    }

    data->blanked = blank_en;
    LOG_INF("LED mock blank[%s]=%u", dev->name, blank_en ? 1U : 0U);
    return 0;
}

int zmk_led_mock_reinit(const struct device *dev) {
    const struct zmk_led_mock_config *config;
    struct zmk_led_mock_data *data;

    if (dev == NULL) {
        return -EINVAL;
    }

    config = dev->config;
    data = dev->data;

    if (config->managed_power_kind == ZMK_LED_MOCK_MANAGED_POWER_NONE) {
        return -ENOTSUP;
    }

    data->blanked = false;
    LOG_INF("LED mock reinit[%s]", dev->name);
    return 0;
}

enum zmk_led_mock_managed_power_kind zmk_led_mock_managed_power_kind(const struct device *dev) {
    const struct zmk_led_mock_config *config;

    if (dev == NULL) {
        return ZMK_LED_MOCK_MANAGED_POWER_NONE;
    }

    config = dev->config;
    return (enum zmk_led_mock_managed_power_kind)config->managed_power_kind;
}

static int zmk_led_mock_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

static DEVICE_API(led, zmk_led_mock_binary_api) = {
    .on = zmk_led_mock_on,
    .off = zmk_led_mock_off,
};

static DEVICE_API(led, zmk_led_mock_brightness_api) = {
    .on = zmk_led_mock_on,
    .off = zmk_led_mock_off,
    .set_brightness = zmk_led_mock_set_brightness,
};

static DEVICE_API(led, zmk_led_mock_color_api) = {
    .on = zmk_led_mock_on,
    .off = zmk_led_mock_off,
    .get_info = zmk_led_mock_get_info,
    .set_color = zmk_led_mock_set_color,
};

static DEVICE_API(led, zmk_led_mock_channels_api) = {
    .on = zmk_led_mock_on,
    .off = zmk_led_mock_off,
    .write_channels = zmk_led_mock_write_channels,
};

static DEVICE_API(led, zmk_led_mock_none_api) = {};

#define COLOR_MAPPING(node_id)                                                                     \
    static const uint8_t color_mapping_##node_id[] = DT_PROP(node_id, color_mapping);
#define LED_INFO(node_id)                                                                          \
    {                                                                                              \
        .label = DT_PROP_OR(node_id, label, ""),                                                   \
        .index = DT_PROP(node_id, index),                                                          \
        .num_colors = DT_PROP_LEN(node_id, color_mapping),                                         \
        .color_mapping = color_mapping_##node_id,                                                  \
    }

#define LED_MOCK_API_PTR(inst)                                                                     \
    ((DT_INST_ENUM_IDX(inst, mode) == ZMK_LED_MOCK_MODE_BINARY)                                    \
         ? &zmk_led_mock_binary_api                                                                \
         : ((DT_INST_ENUM_IDX(inst, mode) == ZMK_LED_MOCK_MODE_BRIGHTNESS)                         \
                ? &zmk_led_mock_brightness_api                                                     \
                : ((DT_INST_ENUM_IDX(inst, mode) == ZMK_LED_MOCK_MODE_COLOR)                       \
                       ? &zmk_led_mock_color_api                                                   \
                       : ((DT_INST_ENUM_IDX(inst, mode) == ZMK_LED_MOCK_MODE_CHANNELS)             \
                              ? &zmk_led_mock_channels_api                                         \
                              : &zmk_led_mock_none_api))))

#define LED_MOCK_INST(inst)                                                                        \
    DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, COLOR_MAPPING)                                         \
    static const struct led_info led_infos_##inst[] = {                                            \
        DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(inst, LED_INFO, (, ))};                              \
    static uint8_t led_mock_brightness_##inst[LED_MOCK_CHILD_ARRAY_LEN(inst)];                     \
    static uint8_t led_mock_colors_##inst[LED_MOCK_CHILD_ARRAY_LEN(inst) * 3U];                    \
    static uint8_t led_mock_channels_##inst[LED_MOCK_CHANNEL_ARRAY_LEN(inst)];                     \
    static struct zmk_led_mock_data led_mock_data_##inst = {                                       \
        .brightness_values = led_mock_brightness_##inst,                                           \
        .color_values = led_mock_colors_##inst,                                                    \
        .channel_values = led_mock_channels_##inst,                                                \
    };                                                                                             \
    static const struct zmk_led_mock_config led_mock_config_##inst = {                             \
        .mode = DT_INST_ENUM_IDX(inst, mode),                                                      \
        .managed_power_kind = DT_INST_ENUM_IDX_OR(inst, managed_power_kind, 0),                    \
        .led_infos = led_infos_##inst,                                                             \
        .led_count = LED_MOCK_CHILD_COUNT(inst),                                                   \
        .channel_count = DT_INST_PROP_OR(inst, channel_count, 0),                                  \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(inst, zmk_led_mock_init, NULL, &led_mock_data_##inst,                    \
                          &led_mock_config_##inst, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,          \
                          LED_MOCK_API_PTR(inst));

DT_INST_FOREACH_STATUS_OKAY(LED_MOCK_INST)
