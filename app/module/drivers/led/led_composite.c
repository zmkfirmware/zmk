/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_composite

#include "led_composite.h"

#include <drivers/ext_power.h>
#include <drivers/led/is31_power.h>
#include <drivers/led/led_mock.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum zmk_led_composite_child_type {
    ZMK_LED_COMPOSITE_CHILD_INVALID = 0,
    ZMK_LED_COMPOSITE_CHILD_MATRIX,
    ZMK_LED_COMPOSITE_CHILD_STRIP,
};

enum zmk_led_composite_matrix_kind {
    ZMK_LED_COMPOSITE_MATRIX_GENERIC = 0,
    ZMK_LED_COMPOSITE_MATRIX_IS31FL3733,
    ZMK_LED_COMPOSITE_MATRIX_IS31FL3737,
    ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3733,
    ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3737,
};

enum zmk_led_composite_matrix_api {
    ZMK_LED_COMPOSITE_MATRIX_API_INVALID = 0,
    ZMK_LED_COMPOSITE_MATRIX_API_CHANNELS,
    ZMK_LED_COMPOSITE_MATRIX_API_BRIGHTNESS,
    ZMK_LED_COMPOSITE_MATRIX_API_BINARY,
};

struct zmk_led_composite_child_config {
    enum zmk_led_composite_child_type type;
    enum zmk_led_composite_matrix_kind matrix_kind;
    struct zmk_is31_power_config is31_power;
    const struct device *dev;
    const struct device *ext_power_dev;
    struct gpio_dt_spec sdb;
    uint32_t channel_offset;
    uint32_t channel_count;
    uint32_t led_count;
    uint32_t pixel_count;
    uint8_t *scalar_channels;
    uint8_t *strip_channels;
    struct led_rgb *strip_pixels;
    bool has_led_device;
    bool has_strip_device;
};

struct zmk_led_composite_config {
    const struct zmk_led_composite_child_config *children;
    size_t child_count;
};

struct zmk_led_composite_child_state {
    enum zmk_led_composite_matrix_api matrix_api;
    bool hard_powered_off;
    bool soft_blanked;
    bool power_state_known;
    bool shutdown_complete;
};

struct zmk_led_composite_data {
    struct k_mutex lock;
    struct zmk_led_composite_child_state *child_states;
    uint32_t total_channel_count;
    uint32_t capabilities;
    const struct device *shared_ext_power_dev;
    bool shared_ext_power_is_off;
    bool shared_ext_power_state_known;
};

#define CHILD_COUNT(node_id) +1
#define LED_COMPOSITE_CHILD_COUNT(inst) (0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, CHILD_COUNT))
#define LED_COMPOSITE_CHILD_ARRAY_LEN(inst)                                                        \
    ((LED_COMPOSITE_CHILD_COUNT(inst) > 0) ? LED_COMPOSITE_CHILD_COUNT(inst) : 1)

#define LED_COMPOSITE_CHILD_HAS_LED(node_id) DT_NODE_HAS_PROP(node_id, led_device)
#define LED_COMPOSITE_CHILD_HAS_STRIP(node_id) DT_NODE_HAS_PROP(node_id, led_strip_device)
#define LED_COMPOSITE_CHILD_LED_NODE(node_id) DT_PHANDLE(node_id, led_device)
#define LED_COMPOSITE_CHILD_STRIP_NODE(node_id) DT_PHANDLE(node_id, led_strip_device)

#define LED_COMPOSITE_CHILD_TYPE(node_id)                                                          \
    (LED_COMPOSITE_CHILD_HAS_LED(node_id) && !LED_COMPOSITE_CHILD_HAS_STRIP(node_id)               \
         ? ZMK_LED_COMPOSITE_CHILD_MATRIX                                                          \
         : (!LED_COMPOSITE_CHILD_HAS_LED(node_id) && LED_COMPOSITE_CHILD_HAS_STRIP(node_id)        \
                ? ZMK_LED_COMPOSITE_CHILD_STRIP                                                    \
                : ZMK_LED_COMPOSITE_CHILD_INVALID))

#define LED_COMPOSITE_CHILD_MOCK_POWER_KIND(node_id)                                               \
    DT_ENUM_IDX_OR(LED_COMPOSITE_CHILD_LED_NODE(node_id), managed_power_kind, 0)
#define LED_COMPOSITE_CHILD_MOCK_POWER_NONE 0
#define LED_COMPOSITE_CHILD_MOCK_POWER_IS31FL3733 1
#define LED_COMPOSITE_CHILD_MOCK_POWER_IS31FL3737 2

#define LED_COMPOSITE_CHILD_IS31_POWER_CHIP(node_id)                                               \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_PROP(node_id, led_device),                                                     \
        (COND_CODE_1(DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id), issi_is31fl3733),   \
                     (ZMK_IS31_CHIP_IS31FL3733),                                                   \
                     (COND_CODE_1(DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id),        \
                                                     zmk_is31fl3737),                              \
                                  (ZMK_IS31_CHIP_IS31FL3737), (ZMK_IS31_CHIP_NONE))))),            \
        (ZMK_IS31_CHIP_NONE))

#define LED_COMPOSITE_CHILD_IS31_POWER_BUS(node_id)                                                \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_PROP(node_id, led_device),                                                     \
        (COND_CODE_1(DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id), issi_is31fl3733),   \
                     (I2C_DT_SPEC_GET(LED_COMPOSITE_CHILD_LED_NODE(node_id))),                     \
                     (COND_CODE_1(DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id),        \
                                                     zmk_is31fl3737),                              \
                                  (I2C_DT_SPEC_GET(LED_COMPOSITE_CHILD_LED_NODE(node_id))),        \
                                  ((struct i2c_dt_spec){0}))))),                                   \
        ((struct i2c_dt_spec){0}))

#define LED_COMPOSITE_CHILD_IS31_POWER_CURRENT_LIMIT(node_id)                                      \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_PROP(node_id, led_device),                                                     \
        (COND_CODE_1(                                                                              \
            DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id), issi_is31fl3733),            \
            (DT_PROP_OR(LED_COMPOSITE_CHILD_LED_NODE(node_id), current_limit, 0xFF)),              \
            (COND_CODE_1(                                                                          \
                DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id), zmk_is31fl3737),         \
                (DT_PROP_OR(LED_COMPOSITE_CHILD_LED_NODE(node_id), current_limit, 0xFF)), (0))))), \
        (0))

#define LED_COMPOSITE_CHILD_IS31_POWER_SYNC(node_id)                                               \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_PROP(node_id, led_device),                                                     \
        (COND_CODE_1(DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id), issi_is31fl3733),   \
                     (DT_ENUM_IDX_OR(LED_COMPOSITE_CHILD_LED_NODE(node_id), sync_mode, 0)), (0))), \
        (0))

#define LED_COMPOSITE_CHILD_MATRIX_KIND(node_id)                                                   \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_PROP(node_id, led_device),                                                     \
        (COND_CODE_1(                                                                              \
            DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id), issi_is31fl3733),            \
            (ZMK_LED_COMPOSITE_MATRIX_IS31FL3733),                                                 \
            (COND_CODE_1(                                                                          \
                DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id), zmk_is31fl3737),         \
                (ZMK_LED_COMPOSITE_MATRIX_IS31FL3737),                                             \
                (COND_CODE_1(                                                                      \
                    DT_NODE_HAS_COMPAT(LED_COMPOSITE_CHILD_LED_NODE(node_id), zmk_led_mock),       \
                    (COND_CODE_1(IS_EQ(LED_COMPOSITE_CHILD_MOCK_POWER_KIND(node_id),               \
                                       LED_COMPOSITE_CHILD_MOCK_POWER_IS31FL3733),                 \
                                 (ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3733),                       \
                                 (COND_CODE_1(IS_EQ(LED_COMPOSITE_CHILD_MOCK_POWER_KIND(node_id),  \
                                                    LED_COMPOSITE_CHILD_MOCK_POWER_IS31FL3737),    \
                                              (ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3737),          \
                                              (ZMK_LED_COMPOSITE_MATRIX_GENERIC))))),              \
                    (ZMK_LED_COMPOSITE_MATRIX_GENERIC))))))),                                      \
        (ZMK_LED_COMPOSITE_MATRIX_GENERIC))

#define LED_COMPOSITE_CHILD_DEVICE(node_id)                                                        \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_device),                                             \
                (DEVICE_DT_GET(LED_COMPOSITE_CHILD_LED_NODE(node_id))),                            \
                (COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_strip_device),                          \
                             (DEVICE_DT_GET(LED_COMPOSITE_CHILD_STRIP_NODE(node_id))), (NULL))))

#define LED_COMPOSITE_CHILD_EXT_POWER(node_id)                                                     \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, ext_power_device),                                       \
                (DEVICE_DT_GET(DT_PHANDLE(node_id, ext_power_device))), (NULL))

#define LED_COMPOSITE_CHILD_SDB(node_id)                                                           \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_device),                                             \
                (GPIO_DT_SPEC_GET_OR(LED_COMPOSITE_CHILD_LED_NODE(node_id), sdb_gpios, {})),       \
                ((struct gpio_dt_spec){0}))

#define LED_COMPOSITE_CHILD_LED_COUNT(node_id)                                                     \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_device),                                             \
                (DT_CHILD_NUM_STATUS_OKAY(LED_COMPOSITE_CHILD_LED_NODE(node_id))), (0))

#define LED_COMPOSITE_STRIP_CHANNEL_LEN(node_id)                                                   \
    ((DT_PROP(node_id, channel_count) > 0) ? DT_PROP(node_id, channel_count) : 1)
#define LED_COMPOSITE_STRIP_PIXEL_LEN(node_id)                                                     \
    (((DT_PROP(node_id, channel_count) / 3U) > 0) ? (DT_PROP(node_id, channel_count) / 3U) : 1)

#define LED_COMPOSITE_SCALAR_CHANNEL_STORAGE(node_id)                                              \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_device),                                             \
                (static uint8_t zmk_led_composite_scalar_channels_##node_id                        \
                     [LED_COMPOSITE_STRIP_CHANNEL_LEN(node_id)];),                                 \
                ())

#define LED_COMPOSITE_STRIP_CHANNEL_STORAGE(node_id)                                               \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_strip_device),                                       \
                (static uint8_t zmk_led_composite_strip_channels_##node_id                         \
                     [LED_COMPOSITE_STRIP_CHANNEL_LEN(node_id)];),                                 \
                ())

#define LED_COMPOSITE_STRIP_PIXEL_STORAGE(node_id)                                                 \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_PROP(node_id, led_strip_device),                                               \
        (static struct led_rgb                                                                     \
             zmk_led_composite_strip_pixels_##node_id[LED_COMPOSITE_STRIP_PIXEL_LEN(node_id)];),   \
        ())

#define LED_COMPOSITE_CHILD_STRIP_CHANNEL_PTR(node_id)                                             \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_strip_device),                                       \
                (zmk_led_composite_strip_channels_##node_id), (NULL))

#define LED_COMPOSITE_CHILD_SCALAR_CHANNEL_PTR(node_id)                                            \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_device),                                             \
                (zmk_led_composite_scalar_channels_##node_id), (NULL))

#define LED_COMPOSITE_CHILD_STRIP_PIXEL_PTR(node_id)                                               \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, led_strip_device),                                       \
                (zmk_led_composite_strip_pixels_##node_id), (NULL))

#define CHILD_CONFIG(node_id)                                                                      \
    {                                                                                              \
        .type = LED_COMPOSITE_CHILD_TYPE(node_id),                                                 \
        .matrix_kind = LED_COMPOSITE_CHILD_MATRIX_KIND(node_id),                                   \
        .is31_power =                                                                              \
            {                                                                                      \
                .chip = LED_COMPOSITE_CHILD_IS31_POWER_CHIP(node_id),                              \
                .dev = LED_COMPOSITE_CHILD_DEVICE(node_id),                                        \
                .bus = LED_COMPOSITE_CHILD_IS31_POWER_BUS(node_id),                                \
                .sdb = LED_COMPOSITE_CHILD_SDB(node_id),                                           \
                .current_limit = LED_COMPOSITE_CHILD_IS31_POWER_CURRENT_LIMIT(node_id),            \
                .sync = LED_COMPOSITE_CHILD_IS31_POWER_SYNC(node_id),                              \
            },                                                                                     \
        .dev = LED_COMPOSITE_CHILD_DEVICE(node_id),                                                \
        .ext_power_dev = LED_COMPOSITE_CHILD_EXT_POWER(node_id),                                   \
        .sdb = LED_COMPOSITE_CHILD_SDB(node_id),                                                   \
        .channel_offset = DT_PROP(node_id, channel_offset),                                        \
        .channel_count = DT_PROP(node_id, channel_count),                                          \
        .led_count = LED_COMPOSITE_CHILD_LED_COUNT(node_id),                                       \
        .pixel_count = DT_PROP(node_id, channel_count) / 3U,                                       \
        .scalar_channels = LED_COMPOSITE_CHILD_SCALAR_CHANNEL_PTR(node_id),                        \
        .strip_channels = LED_COMPOSITE_CHILD_STRIP_CHANNEL_PTR(node_id),                          \
        .strip_pixels = LED_COMPOSITE_CHILD_STRIP_PIXEL_PTR(node_id),                              \
        .has_led_device = LED_COMPOSITE_CHILD_HAS_LED(node_id),                                    \
        .has_strip_device = LED_COMPOSITE_CHILD_HAS_STRIP(node_id),                                \
    },

static inline void zmk_led_composite_lock(struct zmk_led_composite_data *data) {
    k_mutex_lock(&data->lock, K_FOREVER);
}

static inline void zmk_led_composite_unlock(struct zmk_led_composite_data *data) {
    k_mutex_unlock(&data->lock);
}

static bool
zmk_led_composite_child_supports_soft_power(const struct zmk_led_composite_child_config *child) {
    return child->type == ZMK_LED_COMPOSITE_CHILD_MATRIX &&
           (child->matrix_kind == ZMK_LED_COMPOSITE_MATRIX_IS31FL3733 ||
            child->matrix_kind == ZMK_LED_COMPOSITE_MATRIX_IS31FL3737
#if IS_ENABLED(CONFIG_ZMK_LED_MOCK)
            || child->matrix_kind == ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3733 ||
            child->matrix_kind == ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3737
#endif
           );
}

static bool
zmk_led_composite_child_supports_hard_power(const struct zmk_led_composite_child_config *child) {
    if (child->type == ZMK_LED_COMPOSITE_CHILD_MATRIX) {
        return zmk_led_composite_child_supports_soft_power(child) && child->sdb.port != NULL;
    }

    return child->type == ZMK_LED_COMPOSITE_CHILD_STRIP && child->ext_power_dev != NULL;
}

static int zmk_led_composite_child_set_sdb(const struct zmk_led_composite_child_config *child,
                                           bool enable);

static int zmk_led_composite_child_blank(const struct zmk_led_composite_child_config *child,
                                         bool blank_en) {
    switch (child->matrix_kind) {
    case ZMK_LED_COMPOSITE_MATRIX_IS31FL3733:
    case ZMK_LED_COMPOSITE_MATRIX_IS31FL3737:
        return zmk_is31_blank(&child->is31_power, blank_en);
#if IS_ENABLED(CONFIG_ZMK_LED_MOCK)
    case ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3733:
    case ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3737:
        return zmk_led_mock_blank(child->dev, blank_en);
#endif
    default:
        return 0;
    }
}

static int
zmk_led_composite_child_enter_hard_off(const struct zmk_led_composite_child_config *child) {
    switch (child->matrix_kind) {
    case ZMK_LED_COMPOSITE_MATRIX_IS31FL3733:
    case ZMK_LED_COMPOSITE_MATRIX_IS31FL3737:
        return zmk_is31_enter_hard_off(&child->is31_power);
#if IS_ENABLED(CONFIG_ZMK_LED_MOCK)
    case ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3733:
    case ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3737:
        if (child->sdb.port == NULL) {
            return zmk_led_mock_blank(child->dev, true);
        }

        int blank_err = zmk_led_mock_blank(child->dev, true);
        int sdb_err = zmk_led_composite_child_set_sdb(child, false);
        if (blank_err != 0) {
            LOG_WRN("LED composite mock software shutdown failed (%d); SDB result: %d", blank_err,
                    sdb_err);
        }
        return sdb_err;
#endif
    default:
        return 0;
    }
}

static int
zmk_led_composite_child_resume_from_hard_off(const struct zmk_led_composite_child_config *child) {
    switch (child->matrix_kind) {
    case ZMK_LED_COMPOSITE_MATRIX_IS31FL3733:
    case ZMK_LED_COMPOSITE_MATRIX_IS31FL3737:
        return zmk_is31_resume_from_hard_off(&child->is31_power);
#if IS_ENABLED(CONFIG_ZMK_LED_MOCK)
    case ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3733:
    case ZMK_LED_COMPOSITE_MATRIX_MOCK_IS31FL3737: {
        int err = zmk_led_composite_child_set_sdb(child, true);

        if (err != 0) {
            return err;
        }

        return zmk_led_mock_reinit(child->dev);
    }
#endif
    default:
        return 0;
    }
}

static int zmk_led_composite_child_set_sdb(const struct zmk_led_composite_child_config *child,
                                           bool enable) {
    if (child->sdb.port == NULL) {
        return 0;
    }

    return gpio_pin_set_dt(&child->sdb, enable ? 1 : 0);
}

static int zmk_led_composite_flush_strip_child(const struct zmk_led_composite_child_config *child) {
    int err = led_strip_update_channels(child->dev, child->strip_channels, child->channel_count);

    if (err == 0) {
        return 0;
    }

    if (err != -ENOSYS) {
        return err;
    }

    for (uint32_t pixel = 0; pixel < child->pixel_count; pixel++) {
        uint32_t channel = pixel * 3U;

        child->strip_pixels[pixel].r = child->strip_channels[channel];
        child->strip_pixels[pixel].g = child->strip_channels[channel + 1U];
        child->strip_pixels[pixel].b = child->strip_channels[channel + 2U];
    }

    return led_strip_update_rgb(child->dev, child->strip_pixels, child->pixel_count);
}

static int zmk_led_composite_flush_scalar_child(const struct zmk_led_composite_child_config *child,
                                                const struct zmk_led_composite_child_state *state,
                                                uint32_t start_channel, uint32_t num_channels,
                                                const uint8_t *buf) {
    memcpy(child->scalar_channels + start_channel, buf, num_channels);

    uint32_t first_led = start_channel / 3U;
    uint32_t last_led = (start_channel + num_channels - 1U) / 3U;

    for (uint32_t led = first_led; led <= last_led; led++) {
        uint32_t channel = led * 3U;
        uint8_t value =
            MAX(child->scalar_channels[channel],
                MAX(child->scalar_channels[channel + 1U], child->scalar_channels[channel + 2U]));
        int err;

        if (state->matrix_api == ZMK_LED_COMPOSITE_MATRIX_API_BRIGHTNESS) {
            uint8_t brightness = (uint8_t)(((uint16_t)value * 100U) / 255U);
            err = led_set_brightness(child->dev, led, brightness);
        } else {
            err = value > 0U ? led_on(child->dev, led) : led_off(child->dev, led);
        }

        if (err != 0) {
            return err;
        }
    }

    return 0;
}

static int zmk_led_composite_validate(const struct device *dev) {
    const struct zmk_led_composite_config *config = dev->config;
    struct zmk_led_composite_data *data = dev->data;
    uint32_t expected_offset = 0U;
    uint32_t capabilities = 0U;
    bool full_hard_off = config->child_count > 0U;
    const struct device *shared_ext_power_dev = NULL;

    if (config->child_count == 0U) {
        LOG_ERR("LED composite \"%s\" requires at least one child backend", dev->name);
        return -EINVAL;
    }

    for (size_t i = 0; i < config->child_count; i++) {
        const struct zmk_led_composite_child_config *child = &config->children[i];
        struct zmk_led_composite_child_state *state = &data->child_states[i];

        if (child->type == ZMK_LED_COMPOSITE_CHILD_INVALID) {
            LOG_ERR("LED composite \"%s\" child %u must define exactly one of led-device or "
                    "led-strip-device",
                    dev->name, (uint32_t)i);
            return -EINVAL;
        }

        if (child->channel_count == 0U) {
            LOG_ERR("LED composite \"%s\" child %u \"%s\" has channel-count 0", dev->name,
                    (uint32_t)i, child->dev != NULL ? child->dev->name : "<none>");
            return -EINVAL;
        }

        if (child->channel_offset != expected_offset) {
            LOG_ERR("LED composite \"%s\" child %u \"%s\" offset %u does not match expected %u",
                    dev->name, (uint32_t)i, child->dev != NULL ? child->dev->name : "<none>",
                    child->channel_offset, expected_offset);
            return -EINVAL;
        }

        if (child->dev == NULL) {
            LOG_ERR("LED composite \"%s\" child %u has no target device", dev->name, (uint32_t)i);
            return -EINVAL;
        }

        if (!device_is_ready(child->dev)) {
            LOG_ERR("LED composite \"%s\" child %u \"%s\" is not ready", dev->name, (uint32_t)i,
                    child->dev->name);
            return -ENODEV;
        }

        if (child->type == ZMK_LED_COMPOSITE_CHILD_MATRIX) {
            const struct led_driver_api *api = (const struct led_driver_api *)child->dev->api;

            if (child->ext_power_dev != NULL) {
                LOG_ERR("LED composite \"%s\" child %u \"%s\" cannot use ext-power-device with "
                        "led-device",
                        dev->name, (uint32_t)i, child->dev->name);
                return -EINVAL;
            }

            if (api == NULL) {
                LOG_ERR("LED composite \"%s\" child %u \"%s\" has no LED API", dev->name,
                        (uint32_t)i, child->dev->name);
                return -ENOTSUP;
            }

            if (api->write_channels != NULL) {
                state->matrix_api = ZMK_LED_COMPOSITE_MATRIX_API_CHANNELS;
            } else if (api->set_brightness != NULL) {
                state->matrix_api = ZMK_LED_COMPOSITE_MATRIX_API_BRIGHTNESS;
            } else if (api->on != NULL && api->off != NULL) {
                state->matrix_api = ZMK_LED_COMPOSITE_MATRIX_API_BINARY;
            } else {
                LOG_ERR("LED composite \"%s\" child %u \"%s\" lacks "
                        "write_channels/set_brightness/on-off",
                        dev->name, (uint32_t)i, child->dev->name);
                return -ENOTSUP;
            }

            if (state->matrix_api != ZMK_LED_COMPOSITE_MATRIX_API_CHANNELS) {
                if (child->led_count == 0U) {
                    LOG_ERR("LED composite \"%s\" scalar child %u \"%s\" has no child LEDs",
                            dev->name, (uint32_t)i, child->dev->name);
                    return -EINVAL;
                }

                if (child->channel_count != child->led_count * 3U) {
                    LOG_ERR("LED composite \"%s\" scalar child %u \"%s\" channel-count %u "
                            "must equal three times LED count %u",
                            dev->name, (uint32_t)i, child->dev->name, child->channel_count,
                            child->led_count);
                    return -EINVAL;
                }

                if (child->scalar_channels == NULL) {
                    LOG_ERR("LED composite \"%s\" scalar child %u \"%s\" has no channel buffer",
                            dev->name, (uint32_t)i, child->dev->name);
                    return -EINVAL;
                }
            }

            if (child->sdb.port != NULL && !gpio_is_ready_dt(&child->sdb)) {
                LOG_ERR("LED composite \"%s\" child %u \"%s\" SDB GPIO is not ready", dev->name,
                        (uint32_t)i, child->dev->name);
                return -ENODEV;
            }

            if (zmk_led_composite_child_supports_soft_power(child)) {
                capabilities |= ZMK_LED_COMPOSITE_CAP_MANAGED_POWER;
            }

            if (!zmk_led_composite_child_supports_hard_power(child)) {
                full_hard_off = false;
            }
        } else {
            const struct led_strip_driver_api *api =
                (const struct led_strip_driver_api *)child->dev->api;
            size_t strip_len;

            if (api == NULL || api->update_rgb == NULL || api->length == NULL) {
                LOG_ERR("LED composite \"%s\" child %u \"%s\" lacks LED strip API", dev->name,
                        (uint32_t)i, child->dev->name);
                return -ENOTSUP;
            }

            if ((child->channel_count % 3U) != 0U) {
                LOG_ERR("LED composite \"%s\" child %u \"%s\" channel-count %u is not divisible "
                        "by 3",
                        dev->name, (uint32_t)i, child->dev->name, child->channel_count);
                return -EINVAL;
            }

            if (child->strip_channels == NULL || child->strip_pixels == NULL) {
                LOG_ERR("LED composite \"%s\" child %u \"%s\" is missing strip buffers", dev->name,
                        (uint32_t)i, child->dev->name);
                return -EINVAL;
            }

            strip_len = led_strip_length(child->dev);
            if (strip_len < child->pixel_count) {
                LOG_ERR("LED composite \"%s\" child %u \"%s\" strip length %u is smaller than "
                        "required pixels %u",
                        dev->name, (uint32_t)i, child->dev->name, (uint32_t)strip_len,
                        child->pixel_count);
                return -EINVAL;
            }

            if (child->ext_power_dev != NULL) {
                if (!device_is_ready(child->ext_power_dev)) {
                    LOG_ERR("LED composite \"%s\" child %u \"%s\" ext-power device \"%s\" is "
                            "not ready",
                            dev->name, (uint32_t)i, child->dev->name, child->ext_power_dev->name);
                    return -ENODEV;
                }

                if (shared_ext_power_dev == NULL) {
                    shared_ext_power_dev = child->ext_power_dev;
                } else if (shared_ext_power_dev != child->ext_power_dev) {
                    LOG_ERR("LED composite \"%s\" strip children must share the same ext-power "
                            "device",
                            dev->name);
                    return -EINVAL;
                }

                capabilities |= ZMK_LED_COMPOSITE_CAP_MANAGED_POWER;
            } else {
                full_hard_off = false;
            }
        }

        /* A ready child has completed its own initialization. The shared rail is
         * reconciled explicitly on the first power operation. */
        state->power_state_known = child->ext_power_dev == NULL;
        expected_offset += child->channel_count;
    }

    if ((capabilities & ZMK_LED_COMPOSITE_CAP_MANAGED_POWER) != 0U && full_hard_off) {
        capabilities |= ZMK_LED_COMPOSITE_CAP_FULL_HARD_OFF;
    }

    data->total_channel_count = expected_offset;
    data->capabilities = capabilities;
    data->shared_ext_power_dev = shared_ext_power_dev;
    data->shared_ext_power_is_off = false;
    data->shared_ext_power_state_known = false;
    return 0;
}

int zmk_led_composite_get_channel_count(const struct device *dev, uint32_t *count) {
    const struct zmk_led_composite_data *data;

    if (dev == NULL || count == NULL) {
        return -EINVAL;
    }

    if (!device_is_ready(dev)) {
        return -ENODEV;
    }

    data = dev->data;
    *count = data->total_channel_count;
    return 0;
}

int zmk_led_composite_get_capabilities(const struct device *dev, uint32_t *caps) {
    const struct zmk_led_composite_data *data;

    if (dev == NULL || caps == NULL) {
        return -EINVAL;
    }

    if (!device_is_ready(dev)) {
        return -ENODEV;
    }

    data = dev->data;
    *caps = data->capabilities;
    return 0;
}

static int zmk_led_composite_write_channels(const struct device *dev, uint32_t start_channel,
                                            uint32_t num_channels, const uint8_t *buf) {
    const struct zmk_led_composite_config *config = dev->config;
    struct zmk_led_composite_data *data = dev->data;
    uint32_t cursor = start_channel;
    uint32_t remaining = num_channels;
    const uint8_t *cursor_buf = buf;
    int rc = 0;

    if ((buf == NULL && num_channels > 0U) || start_channel > data->total_channel_count ||
        num_channels > (data->total_channel_count - start_channel)) {
        return -EINVAL;
    }

    if (num_channels == 0U) {
        return 0;
    }

    zmk_led_composite_lock(data);

    for (size_t i = 0; i < config->child_count; i++) {
        const struct zmk_led_composite_child_config *child = &config->children[i];
        uint32_t child_end = child->channel_offset + child->channel_count;
        uint32_t chunk_len;
        int err;

        if (cursor >= child_end) {
            continue;
        }

        if (cursor < child->channel_offset) {
            LOG_ERR("LED composite \"%s\" internal gap before child %u \"%s\"", dev->name,
                    (uint32_t)i, child->dev->name);
            rc = -EINVAL;
            break;
        }

        chunk_len = MIN(remaining, child_end - cursor);
        /* A partial or failed write can leave visible output behind. */
        data->child_states[i].shutdown_complete = false;

        if (child->type == ZMK_LED_COMPOSITE_CHILD_MATRIX) {
            struct zmk_led_composite_child_state *state = &data->child_states[i];
            uint32_t child_start = cursor - child->channel_offset;

            if (state->matrix_api == ZMK_LED_COMPOSITE_MATRIX_API_CHANNELS) {
                err = led_write_channels(child->dev, child_start, chunk_len, cursor_buf);
            } else {
                err = zmk_led_composite_flush_scalar_child(child, state, child_start, chunk_len,
                                                           cursor_buf);
            }
        } else {
            memcpy(child->strip_channels + (cursor - child->channel_offset), cursor_buf, chunk_len);
            err = zmk_led_composite_flush_strip_child(child);
        }

        if (err != 0) {
            LOG_ERR("LED composite \"%s\" failed writing child %u \"%s\" (%d)", dev->name,
                    (uint32_t)i, child->dev->name, err);
            rc = err;
            break;
        }

        cursor += chunk_len;
        cursor_buf += chunk_len;
        remaining -= chunk_len;
        if (remaining == 0U) {
            break;
        }
    }

    if (rc == 0 && remaining != 0U) {
        LOG_ERR("LED composite \"%s\" could not map %u trailing channels", dev->name, remaining);
        rc = -EINVAL;
    }

    zmk_led_composite_unlock(data);
    return rc;
}

/* Clear every scalar LED even when an earlier LED fails. A failed frame must
 * not strand the remaining GPIO/PWM outputs in their previous state. */
static int zmk_led_composite_clear_child_locked(const struct zmk_led_composite_child_config *child,
                                                const struct zmk_led_composite_child_state *state) {
    if (child->type == ZMK_LED_COMPOSITE_CHILD_STRIP) {
        memset(child->strip_channels, 0, child->channel_count);
        return zmk_led_composite_flush_strip_child(child);
    }

    memset(child->scalar_channels, 0, child->channel_count);
    if (state->matrix_api == ZMK_LED_COMPOSITE_MATRIX_API_CHANNELS) {
        return led_write_channels(child->dev, 0, child->channel_count, child->scalar_channels);
    }

    int first_err = 0;
    for (uint32_t led = 0; led < child->led_count; led++) {
        int err = state->matrix_api == ZMK_LED_COMPOSITE_MATRIX_API_BRIGHTNESS
                      ? led_set_brightness(child->dev, led, 0)
                      : led_off(child->dev, led);
        if (first_err == 0 && err != 0) {
            first_err = err;
        }
    }
    return first_err;
}

static int zmk_led_composite_shutdown_locked(const struct device *dev) {
    const struct zmk_led_composite_config *config = dev->config;
    struct zmk_led_composite_data *data = dev->data;
    int first_err = 0;

    for (size_t i = 0; i < config->child_count; i++) {
        const struct zmk_led_composite_child_config *child = &config->children[i];
        struct zmk_led_composite_child_state *state = &data->child_states[i];

        if (state->shutdown_complete) {
            continue;
        }

        if (child->type == ZMK_LED_COMPOSITE_CHILD_STRIP && child->ext_power_dev != NULL &&
            data->shared_ext_power_state_known && data->shared_ext_power_is_off) {
            state->power_state_known = true;
            state->hard_powered_off = true;
            state->shutdown_complete = true;
            continue;
        }

        /* Do not access a child already confirmed to be electrically disabled.
         * In particular, retrying another child's failure must not wake an IS31. */
        bool already_disabled =
            state->power_state_known && (state->hard_powered_off || state->soft_blanked);
        int clear_err = already_disabled ? 0 : zmk_led_composite_clear_child_locked(child, state);
        int err = clear_err;

        if (child->type == ZMK_LED_COMPOSITE_CHILD_STRIP && child->ext_power_dev != NULL) {
            /* The shared rail is attempted once, after all children. Its success
             * is sufficient even when this strip's clear failed. */
            if (clear_err != 0) {
                LOG_WRN("LED composite \"%s\" strip child %u clear failed (%d); trying power off",
                        dev->name, (uint32_t)i, clear_err);
            }
            continue;
        }

        bool hard = zmk_led_composite_child_supports_hard_power(child);
        bool soft = zmk_led_composite_child_supports_soft_power(child);
        if (hard) {
            err = state->power_state_known && state->hard_powered_off
                      ? 0
                      : zmk_led_composite_child_enter_hard_off(child);
        } else if (soft) {
            err = already_disabled ? 0 : zmk_led_composite_child_blank(child, true);
        }

        if (err == 0) {
            state->power_state_known = true;
            state->hard_powered_off = hard;
            state->soft_blanked = soft;
            state->shutdown_complete = true;
            if (clear_err != 0) {
                LOG_WRN("LED composite \"%s\" child %u clear failed (%d), but shutdown succeeded",
                        dev->name, (uint32_t)i, clear_err);
            }
        } else {
            state->power_state_known = false;
            state->hard_powered_off = false;
            state->soft_blanked = false;
            LOG_ERR("LED composite \"%s\" failed shutting down child %u \"%s\" (%d)", dev->name,
                    (uint32_t)i, child->dev->name, err);
            if (first_err == 0) {
                first_err = err;
            }
        }
    }

    if (data->shared_ext_power_dev != NULL &&
        (!data->shared_ext_power_state_known || !data->shared_ext_power_is_off)) {
        int err = ext_power_disable_runtime(data->shared_ext_power_dev);
        data->shared_ext_power_state_known = err == 0;
        data->shared_ext_power_is_off = err == 0;
        if (err != 0) {
            LOG_ERR("LED composite \"%s\" failed disabling ext-power \"%s\" (%d)", dev->name,
                    data->shared_ext_power_dev->name, err);
            if (first_err == 0) {
                first_err = err;
            }
        }

        for (size_t i = 0; i < config->child_count; i++) {
            const struct zmk_led_composite_child_config *child = &config->children[i];
            struct zmk_led_composite_child_state *state = &data->child_states[i];
            if (child->type == ZMK_LED_COMPOSITE_CHILD_STRIP && child->ext_power_dev != NULL) {
                state->power_state_known = err == 0;
                state->hard_powered_off = err == 0;
                state->shutdown_complete = err == 0;
            }
        }
    }

    return first_err;
}

static int zmk_led_composite_power_locked(const struct device *dev, bool enable) {
    const struct zmk_led_composite_config *config = dev->config;
    struct zmk_led_composite_data *data = dev->data;
    int err;
    int off_err;

    if (!enable) {
        return zmk_led_composite_shutdown_locked(dev);
    }

    for (size_t i = 0; i < config->child_count; i++) {
        const struct zmk_led_composite_child_config *child = &config->children[i];
        struct zmk_led_composite_child_state *state = &data->child_states[i];

        if (!zmk_led_composite_child_supports_soft_power(child)) {
            continue;
        }

        if (!state->power_state_known || state->hard_powered_off) {
            state->power_state_known = false;
            state->shutdown_complete = false;
            err = zmk_led_composite_child_resume_from_hard_off(child);
        } else if (state->soft_blanked) {
            state->power_state_known = false;
            state->shutdown_complete = false;
            err = zmk_led_composite_child_blank(child, false);
        } else {
            continue;
        }

        if (err != 0) {
            LOG_ERR("LED composite \"%s\" failed resuming child %u \"%s\" (%d)", dev->name,
                    (uint32_t)i, child->dev->name, err);
            goto rollback;
        }

        state->power_state_known = true;
        state->hard_powered_off = false;
        state->soft_blanked = false;
    }

    /* Do not supply the strip rail until every matrix child has resumed. */
    if (data->shared_ext_power_dev != NULL &&
        (!data->shared_ext_power_state_known || data->shared_ext_power_is_off)) {
        data->shared_ext_power_state_known = false;
        err = ext_power_enable_runtime(data->shared_ext_power_dev);
        if (err != 0) {
            LOG_ERR("LED composite \"%s\" failed enabling ext-power \"%s\" (%d)", dev->name,
                    data->shared_ext_power_dev->name, err);
            goto rollback;
        }
        data->shared_ext_power_state_known = true;
        data->shared_ext_power_is_off = false;
    }

    for (size_t i = 0; i < config->child_count; i++) {
        struct zmk_led_composite_child_state *state = &data->child_states[i];
        state->shutdown_complete = false;
        state->power_state_known = true;
        state->hard_powered_off = false;
        state->soft_blanked = false;
    }
    return 0;

rollback:
    /* One failed resume must not leave earlier children or the shared rail on.
     * Preserve the initiating error; shutdown records each incomplete child. */
    off_err = zmk_led_composite_shutdown_locked(dev);
    if (off_err != 0) {
        LOG_ERR("LED composite \"%s\" resume failed (%d), shutdown rollback failed (%d)", dev->name,
                err, off_err);
    }
    return err;
}

int zmk_led_composite_power(const struct device *dev, bool enable) {
    struct zmk_led_composite_data *data;
    int rc;

    if (dev == NULL) {
        return -EINVAL;
    }

    if (!device_is_ready(dev)) {
        return -ENODEV;
    }

    data = dev->data;
    zmk_led_composite_lock(data);
    rc = zmk_led_composite_power_locked(dev, enable);
    zmk_led_composite_unlock(data);

    return rc;
}

static int zmk_led_composite_init(const struct device *dev) {
    struct zmk_led_composite_data *data = dev->data;

    k_mutex_init(&data->lock);
    memset(data->child_states, 0,
           sizeof(struct zmk_led_composite_child_state) *
               (((const struct zmk_led_composite_config *)dev->config)->child_count));
    return zmk_led_composite_validate(dev);
}

static DEVICE_API(led, zmk_led_composite_api) = {
    .write_channels = zmk_led_composite_write_channels,
};

#define LED_COMPOSITE_INST(inst)                                                                   \
    DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, LED_COMPOSITE_SCALAR_CHANNEL_STORAGE)                  \
    DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, LED_COMPOSITE_STRIP_CHANNEL_STORAGE)                   \
    DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, LED_COMPOSITE_STRIP_PIXEL_STORAGE)                     \
    static const struct zmk_led_composite_child_config                                             \
        zmk_led_composite_children_##inst[LED_COMPOSITE_CHILD_ARRAY_LEN(inst)] = {                 \
            DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, CHILD_CONFIG)};                                \
    static const struct zmk_led_composite_config zmk_led_composite_config_##inst = {               \
        .children = zmk_led_composite_children_##inst,                                             \
        .child_count = LED_COMPOSITE_CHILD_COUNT(inst),                                            \
    };                                                                                             \
    static struct zmk_led_composite_child_state                                                    \
        zmk_led_composite_child_states_##inst[LED_COMPOSITE_CHILD_ARRAY_LEN(inst)];                \
    static struct zmk_led_composite_data zmk_led_composite_data_##inst = {                         \
        .child_states = zmk_led_composite_child_states_##inst,                                     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(inst, zmk_led_composite_init, NULL, &zmk_led_composite_data_##inst,      \
                          &zmk_led_composite_config_##inst, POST_KERNEL, CONFIG_LED_INIT_PRIORITY, \
                          &zmk_led_composite_api);

DT_INST_FOREACH_STATUS_OKAY(LED_COMPOSITE_INST)
