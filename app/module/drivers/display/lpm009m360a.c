/*
 * Copyright (c) 2023 Taisheng WANG <wstrn66@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jdi_lpm009m360a

#include "lpm009m360a.h"

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/display.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_lpm009m360a, CONFIG_DISPLAY_LOG_LEVEL);

#define LPM009M360A_RESET_TIME K_MSEC(1)
#define LPM009M360A_EXIT_SLEEP_TIME K_MSEC(1)

struct lpm009m360a_data {
    uint8_t buf[9 * 144];
};

struct lpm009m360a_config {
    struct spi_dt_spec bus;
    struct gpio_dt_spec extcomin;
    struct gpio_dt_spec disp;
    uint16_t height;
    uint16_t width;
    int rotation;
    int reverse;

    uint8_t color_mode[1];
};

static int lpm009m360a_transmit_hold(const struct device *dev, uint8_t cmd, uint8_t arg,
                                     const uint8_t *tx_data, size_t tx_count) {
    const struct lpm009m360a_config *config = dev->config;
    struct spi_buf tx_buf = {.buf = &cmd, .len = 1};
    struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};
    int ret;

    ret = spi_write_dt(&config->bus, &tx_bufs);
    tx_buf.buf = &arg;
    ret = spi_write_dt(&config->bus, &tx_bufs);
    if (ret < 0) {
        return ret;
    }

    if (tx_data != NULL) {
        tx_buf.buf = (void *)tx_data;
        tx_buf.len = tx_count;
        ret = spi_write_dt(&config->bus, &tx_bufs);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

static int lpm009m360a_transmit(const struct device *dev, uint8_t cmd, uint8_t arg,
                                const uint8_t *tx_data, size_t tx_count) {
    const struct lpm009m360a_config *config = dev->config;
    int ret;

    ret = lpm009m360a_transmit_hold(dev, cmd, arg, tx_data, tx_count);
    spi_release_dt(&config->bus);
    return ret;
}

static int lpm009m360a_exit_sleep(const struct device *dev) {
    int ret;
    const struct lpm009m360a_config *config = dev->config;
    ret = gpio_pin_set_dt(&config->disp, 1);
    if (ret < 0) {
        return ret;
    }
    k_sleep(LPM009M360A_EXIT_SLEEP_TIME);
    return 0;
}

static int lpm009m360a_sleep(const struct device *dev) {
    int ret;
    const struct lpm009m360a_config *config = dev->config;
    ret = gpio_pin_set_dt(&config->disp, 0);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static int lpm009m360a_reset_display(const struct device *dev) {
    int ret;

    LOG_DBG("Resetting display");
    ret = lpm009m360a_transmit(dev, LPM009M360A_CMD_ALL_CLEAR, 0, NULL, 0);

    k_sleep(LPM009M360A_RESET_TIME);

    return 0;
}

static int lpm009m360a_blanking_on(const struct device *dev) { return lpm009m360a_sleep(dev); }

static int lpm009m360a_blanking_off(const struct device *dev) {
    return lpm009m360a_exit_sleep(dev);
}

static int lpm009m360a_read(const struct device *dev, const uint16_t x, const uint16_t y,
                            const struct display_buffer_descriptor *desc, void *buf) {
    return -ENOTSUP;
}

#define RGB565_RGB111(s) ((s & 0x8000) >> 12) | ((s & 0x0400) >> 8) | ((s & 0x0010) >> 3)
static int lpm009m360a_write(const struct device *dev, const uint16_t x, const uint16_t y,
                             const struct display_buffer_descriptor *desc, const void *buf) {
    // const uint16_t *source_buf = (uint16_t *)buf;
    const uint8_t *source_buf8 = (uint8_t *)buf;
    const struct lpm009m360a_config *config = dev->config;
    struct lpm009m360a_data *data = dev->data;
    int ret = 0;
    uint8_t cmd = LPM009M360A_CMD_UPDATE | (config->color_mode[0] << 2);
    size_t len = (config->color_mode[0] == 0x02) ? 9 : 36;
    uint8_t cnt;
    if (config->rotation == 0) {
        cnt = desc->height;
        for (uint8_t i = 0; i < desc->height; i++) {
            for (uint8_t j = 0; j < desc->width; j++) {
                data->buf[(y + i) * 9 + (x / 8 + j)] = source_buf8[i * 9 + j];
            }
        }
        // LOG_INF("x:%d, y:%d, w:%d, h:%d", x, y, desc->width, desc->height);
        for (uint8_t i = 0; i < cnt; i++) {
            ret = lpm009m360a_transmit_hold(dev, cmd, i + y + 1,
                                            (uint8_t *)&data->buf[(y + i) * len], len);
        }
    } else if (config->rotation == 1) {
        cnt = desc->width;
        for (uint8_t i = 0; i < (desc->height) / 8; i++) {
            for (uint8_t j = 0; j < desc->width; j++) {
                data->buf[(143 - x - j) * 9 + y / 8 + i] = source_buf8[i * desc->width + j];
            }
        }
        // LOG_INF("x:%d, y:%d, w:%d, h:%d", x, y, desc->width, desc->height);
        for (uint8_t i = 0; i < cnt; i++) {
            ret = lpm009m360a_transmit_hold(dev, cmd, 143 - x - i + 1,
                                            (uint8_t *)&data->buf[(143 - x - i) * len], len);
        }
    }

    ret = lpm009m360a_transmit_hold(dev, LPM009M360A_CMD_NO_UPDATE, 0, NULL, 0);
    ret = lpm009m360a_transmit_hold(dev, LPM009M360A_CMD_NO_UPDATE, 0, NULL, 0);
    spi_release_dt(&config->bus);
    return ret;
}

static void *lpm009m360a_get_framebuffer(const struct device *dev) { return NULL; }

static int lpm009m360a_set_brightness(const struct device *dev, const uint8_t brightness) {
    return -ENOTSUP;
}

static int lpm009m360a_set_contrast(const struct device *dev, const uint8_t contrast) {
    return -ENOTSUP;
}

static void lpm009m360a_get_capabilities(const struct device *dev,
                                         struct display_capabilities *capabilities) {
    const struct lpm009m360a_config *config = dev->config;

    memset(capabilities, 0, sizeof(struct display_capabilities));
    capabilities->x_resolution = config->width;
    capabilities->y_resolution = config->height;

    capabilities->supported_pixel_formats = PIXEL_FORMAT_MONO01 | PIXEL_FORMAT_MONO10;
    if (config->reverse)
        capabilities->current_pixel_format = PIXEL_FORMAT_MONO01;
    else
        capabilities->current_pixel_format = PIXEL_FORMAT_MONO10;
    if (config->rotation == 0)
        capabilities->screen_info = SCREEN_INFO_X_ALIGNMENT_WIDTH | SCREEN_INFO_MONO_MSB_FIRST;
    else if (config->rotation == 1)
        capabilities->screen_info = SCREEN_INFO_MONO_VTILED | SCREEN_INFO_MONO_MSB_FIRST;
    capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int lpm009m360a_set_pixel_format(const struct device *dev,
                                        const enum display_pixel_format pixel_format) {

    LOG_ERR("Pixel format change not implemented");

    return -ENOTSUP;
}

static int lpm009m360a_set_orientation(const struct device *dev,
                                       const enum display_orientation orientation) {
    if (orientation == DISPLAY_ORIENTATION_NORMAL) {
        return 0;
    }

    LOG_ERR("Changing display orientation not implemented");

    return -ENOTSUP;
}

static int lpm009m360a_init(const struct device *dev) {
    LOG_INF("initializing");
    const struct lpm009m360a_config *config = dev->config;
    int ret;

    // if (!spi_is_ready_dt(&config->bus)) {
    //     LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
    //     return -ENODEV;
    // }

    // if (!gpio_is_ready_dt(&config->extcomin)) {
    //     LOG_ERR("extcomin GPIO port for display not ready");
    //     return -ENODEV;
    // }
    ret = gpio_pin_configure_dt(&config->extcomin, GPIO_OUTPUT_INACTIVE);
    if (ret) {
        LOG_ERR("Couldn't configure extcomin pin");
        return ret;
    }

    // if (!gpio_is_ready_dt(&config->disp)) {
    //     LOG_ERR("disp GPIO port not ready");
    //     return -ENODEV;
    // }
    ret = gpio_pin_configure_dt(&config->disp, GPIO_OUTPUT);
    if (ret) {
        LOG_ERR("Couldn't configure disp pin");
        return ret;
    }
    ret = lpm009m360a_reset_display(dev);
    if (ret) {
        LOG_ERR("Couldn't reset display");
        return ret;
    }
    LOG_INF("initialized");
    return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lpm009m360a_pm_action(const struct device *dev, enum pm_device_action action) {
    int ret = 0;

    switch (action) {
    case PM_DEVICE_ACTION_RESUME:
        LOG_INF("resume");
        lpm009m360a_exit_sleep(dev);
        break;
    case PM_DEVICE_ACTION_SUSPEND:
        LOG_INF("suspend");
        lpm009m360a_sleep(dev);
        break;
    case PM_DEVICE_ACTION_TURN_OFF:
        LOG_INF("turn off");
        break;
    case PM_DEVICE_ACTION_TURN_ON:
        lpm009m360a_init(dev);
        LOG_INF("turn on");
        break;
    default:
        ret = -ENOTSUP;
        break;
    }

    return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_driver_api lpm009m360a_api = {
    .blanking_on = lpm009m360a_blanking_on,
    .blanking_off = lpm009m360a_blanking_off,
    .write = lpm009m360a_write,
    .read = lpm009m360a_read,
    .get_framebuffer = lpm009m360a_get_framebuffer,
    .set_brightness = lpm009m360a_set_brightness,
    .set_contrast = lpm009m360a_set_contrast,
    .get_capabilities = lpm009m360a_get_capabilities,
    .set_pixel_format = lpm009m360a_set_pixel_format,
    .set_orientation = lpm009m360a_set_orientation,
};

#define LPM009M360A_INIT(inst)                                                                     \
                                                                                                   \
    const static struct lpm009m360a_config lpm009m360a_config_##inst = {                           \
        .bus = SPI_DT_SPEC_INST_GET(                                                               \
            inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),         \
        .extcomin = GPIO_DT_SPEC_INST_GET(inst, extcomin_gpios),                                   \
        .disp = GPIO_DT_SPEC_INST_GET(inst, disp_gpios),                                           \
        .width = DT_INST_PROP(inst, width),                                                        \
        .height = DT_INST_PROP(inst, height),                                                      \
        .color_mode = DT_INST_PROP(inst, color_mode),                                              \
        .rotation = DT_INST_PROP(inst, rotation),                                                  \
        .reverse = DT_INST_PROP(inst, reverse),                                                    \
    };                                                                                             \
    static struct lpm009m360a_data lpm009m360a_data_##inst = {0};                                  \
                                                                                                   \
    PM_DEVICE_DT_INST_DEFINE(inst, lpm009m360a_pm_action);                                         \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(inst, lpm009m360a_init, PM_DEVICE_DT_INST_GET(inst),                     \
                          &lpm009m360a_data_##inst, &lpm009m360a_config_##inst, POST_KERNEL,       \
                          CONFIG_DISPLAY_INIT_PRIORITY, &lpm009m360a_api);

DT_INST_FOREACH_STATUS_OKAY(LPM009M360A_INIT)
