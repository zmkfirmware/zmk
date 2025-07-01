/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbHH, Peter Johanson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gooddisplay_il0323

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

#include "il0323_regs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(il0323, CONFIG_DISPLAY_LOG_LEVEL);

/**
 * IL0323 compatible EPD controller driver.
 *
 */

#define EPD_PANEL_WIDTH DT_INST_PROP(0, width)
#define EPD_PANEL_HEIGHT DT_INST_PROP(0, height)
#define IL0323_PIXELS_PER_BYTE 8U

/* Horizontally aligned page! */
#define IL0323_NUMOF_PAGES (EPD_PANEL_WIDTH / IL0323_PIXELS_PER_BYTE)
#define IL0323_PANEL_FIRST_GATE 0U
#define IL0323_PANEL_LAST_GATE (EPD_PANEL_HEIGHT - 1)
#define IL0323_PANEL_FIRST_PAGE 0U
#define IL0323_PANEL_LAST_PAGE (IL0323_NUMOF_PAGES - 1)
#define IL0323_BUFFER_SIZE 1280

struct il0323_cfg {
    struct gpio_dt_spec reset;
    struct gpio_dt_spec dc;
    struct gpio_dt_spec busy;
    struct spi_dt_spec spi;
};

static uint8_t il0323_pwr[] = DT_INST_PROP(0, pwr);

static uint8_t last_buffer[IL0323_BUFFER_SIZE];
static bool blanking_on = true;
static bool init_clear_done = false;

static inline int il0323_write_cmd(const struct il0323_cfg *cfg, uint8_t cmd, uint8_t *data,
                                   size_t len) {
    struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
    struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};

    gpio_pin_set_dt(&cfg->dc, 1);
    if (spi_write_dt(&cfg->spi, &buf_set)) {
        return -EIO;
    }

    if (data != NULL) {
        buf.buf = data;
        buf.len = len;
        gpio_pin_set_dt(&cfg->dc, 0);
        if (spi_write_dt(&cfg->spi, &buf_set)) {
            return -EIO;
        }
    }

    return 0;
}

static inline void il0323_busy_wait(const struct il0323_cfg *cfg) {
    int pin = gpio_pin_get_dt(&cfg->busy);

    while (pin > 0) {
        __ASSERT(pin >= 0, "Failed to get pin level");
        // LOG_DBG("wait %u", pin);
        k_msleep(IL0323_BUSY_DELAY);
        pin = gpio_pin_get_dt(&cfg->busy);
    }
}

static int il0323_update_display(const struct device *dev) {
    const struct il0323_cfg *cfg = dev->config;

    LOG_DBG("Trigger update sequence");
    if (il0323_write_cmd(cfg, IL0323_CMD_DRF, NULL, 0)) {
        return -EIO;
    }

    k_msleep(IL0323_BUSY_DELAY);

    return 0;
}

static int il0323_write(const struct device *dev, const uint16_t x, const uint16_t y,
                        const struct display_buffer_descriptor *desc, const void *buf) {
    const struct il0323_cfg *cfg = dev->config;
    uint16_t x_end_idx = x + desc->width - 1;
    uint16_t y_end_idx = y + desc->height - 1;
    uint8_t ptl[IL0323_PTL_REG_LENGTH] = {0};
    size_t buf_len;

    LOG_DBG("x %u, y %u, height %u, width %u, pitch %u", x, y, desc->height, desc->width,
            desc->pitch);

    buf_len = MIN(desc->buf_size, desc->height * desc->width / IL0323_PIXELS_PER_BYTE);
    __ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
    __ASSERT(buf != NULL, "Buffer is not available");
    __ASSERT(buf_len != 0U, "Buffer of length zero");
    __ASSERT(!(desc->width % IL0323_PIXELS_PER_BYTE), "Buffer width not multiple of %d",
             IL0323_PIXELS_PER_BYTE);

    LOG_DBG("buf_len %d", buf_len);
    if ((y_end_idx > (EPD_PANEL_HEIGHT - 1)) || (x_end_idx > (EPD_PANEL_WIDTH - 1))) {
        LOG_ERR("Position out of bounds");
        return -EINVAL;
    }

    /* Setup Partial Window and enable Partial Mode */
    ptl[IL0323_PTL_HRST_IDX] = x;
    ptl[IL0323_PTL_HRED_IDX] = x_end_idx;
    ptl[IL0323_PTL_VRST_IDX] = y;
    ptl[IL0323_PTL_VRED_IDX] = y_end_idx;
    ptl[sizeof(ptl) - 1] = IL0323_PTL_PT_SCAN;
    LOG_HEXDUMP_DBG(ptl, sizeof(ptl), "ptl");

    il0323_busy_wait(cfg);
    if (il0323_write_cmd(cfg, IL0323_CMD_PIN, NULL, 0)) {
        return -EIO;
    }

    if (il0323_write_cmd(cfg, IL0323_CMD_PTL, ptl, sizeof(ptl))) {
        return -EIO;
    }

    if (il0323_write_cmd(cfg, IL0323_CMD_DTM1, last_buffer, IL0323_BUFFER_SIZE)) {
        return -EIO;
    }

    if (il0323_write_cmd(cfg, IL0323_CMD_DTM2, (uint8_t *)buf, buf_len)) {
        return -EIO;
    }

    memcpy(last_buffer, (uint8_t *)buf, IL0323_BUFFER_SIZE);

    /* Update partial window and disable Partial Mode */
    if (blanking_on == false) {
        if (il0323_update_display(dev)) {
            return -EIO;
        }
    }

    if (il0323_write_cmd(cfg, IL0323_CMD_POUT, NULL, 0)) {
        return -EIO;
    }

    return 0;
}

static int il0323_read(const struct device *dev, const uint16_t x, const uint16_t y,
                       const struct display_buffer_descriptor *desc, void *buf) {
    LOG_ERR("not supported");
    return -ENOTSUP;
}

static int il0323_clear_and_write_buffer(const struct device *dev, uint8_t pattern, bool update) {
    struct display_buffer_descriptor desc = {
        .buf_size = IL0323_NUMOF_PAGES,
        .width = EPD_PANEL_WIDTH,
        .height = 1,
        .pitch = EPD_PANEL_WIDTH,
    };
    uint8_t *line;

    line = k_malloc(IL0323_NUMOF_PAGES);
    if (line == NULL) {
        LOG_ERR("Failed to allocate memory for the clear");
        return -ENOMEM;
    }

    memset(line, pattern, IL0323_NUMOF_PAGES);
    for (int i = 0; i < EPD_PANEL_HEIGHT; i++) {
        il0323_write(dev, 0, i, &desc, line);
    }

    k_free(line);

    if (update == true) {
        if (il0323_update_display(dev)) {
            return -EIO;
        }
    }

    return 0;
}

static int il0323_blanking_off(const struct device *dev) {
    const struct il0323_cfg *cfg = dev->config;

    if (!init_clear_done) {
        /* Update EPD panel in normal mode */
        il0323_busy_wait(cfg);
        if (il0323_clear_and_write_buffer(dev, 0xff, true)) {
            return -EIO;
        }
        init_clear_done = true;
    }

    blanking_on = false;

    if (il0323_update_display(dev)) {
        return -EIO;
    }

    return 0;
}

static int il0323_blanking_on(const struct device *dev) {
    blanking_on = true;

    return 0;
}

static void *il0323_get_framebuffer(const struct device *dev) {
    LOG_ERR("not supported");
    return NULL;
}

static int il0323_set_brightness(const struct device *dev, const uint8_t brightness) {
    LOG_WRN("not supported");
    return -ENOTSUP;
}

static int il0323_set_contrast(const struct device *dev, uint8_t contrast) {
    LOG_WRN("not supported");
    return -ENOTSUP;
}

static void il0323_get_capabilities(const struct device *dev, struct display_capabilities *caps) {
    memset(caps, 0, sizeof(struct display_capabilities));
    caps->x_resolution = EPD_PANEL_WIDTH;
    caps->y_resolution = EPD_PANEL_HEIGHT;
    caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
    caps->current_pixel_format = PIXEL_FORMAT_MONO10;
    caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;
}

static int il0323_set_orientation(const struct device *dev,
                                  const enum display_orientation orientation) {
    LOG_ERR("Unsupported");
    return -ENOTSUP;
}

static int il0323_set_pixel_format(const struct device *dev, const enum display_pixel_format pf) {
    if (pf == PIXEL_FORMAT_MONO10) {
        return 0;
    }

    LOG_ERR("not supported");
    return -ENOTSUP;
}

static int il0323_controller_init(const struct device *dev) {
    const struct il0323_cfg *cfg = dev->config;
    uint8_t tmp[IL0323_TRES_REG_LENGTH];

    LOG_DBG("");

    gpio_pin_set_dt(&cfg->reset, 1);
    k_msleep(IL0323_RESET_DELAY);
    gpio_pin_set_dt(&cfg->reset, 0);
    k_msleep(IL0323_RESET_DELAY);
    il0323_busy_wait(cfg);

    LOG_DBG("Initialize IL0323 controller");

    if (il0323_write_cmd(cfg, IL0323_CMD_PWR, il0323_pwr, sizeof(il0323_pwr))) {
        return -EIO;
    }

    /* Turn on: booster, controller, regulators, and sensor. */
    if (il0323_write_cmd(cfg, IL0323_CMD_PON, NULL, 0)) {
        return -EIO;
    }

    k_msleep(IL0323_PON_DELAY);
    il0323_busy_wait(cfg);

    /* Pannel settings, KW mode */
    tmp[0] = IL0323_PSR_UD | IL0323_PSR_SHL | IL0323_PSR_SHD | IL0323_PSR_RST;
#if EPD_PANEL_WIDTH == 80

#if EPD_PANEL_HEIGHT == 128
    tmp[0] |= IL0323_PSR_RES_HEIGHT;
#endif /* panel height */

#else
    tmp[0] |= IL0323_PSR_RES_WIDTH;
#if EPD_PANEL_HEIGHT == 96
    tmp[0] |= IL0323_PSR_RES_HEIGHT;
#else
#endif /* panel height */

#endif /* panel width */

    LOG_HEXDUMP_DBG(tmp, 1, "PSR");
    if (il0323_write_cmd(cfg, IL0323_CMD_PSR, tmp, 1)) {
        return -EIO;
    }

    /* Set panel resolution */
    tmp[IL0323_TRES_HRES_IDX] = EPD_PANEL_WIDTH;
    tmp[IL0323_TRES_VRES_IDX] = EPD_PANEL_HEIGHT;
    LOG_HEXDUMP_DBG(tmp, IL0323_TRES_REG_LENGTH, "TRES");
    if (il0323_write_cmd(cfg, IL0323_CMD_TRES, tmp, IL0323_TRES_REG_LENGTH)) {
        return -EIO;
    }

    tmp[IL0323_CDI_CDI_IDX] = DT_INST_PROP(0, cdi);
    LOG_HEXDUMP_DBG(tmp, IL0323_CDI_REG_LENGTH, "CDI");
    if (il0323_write_cmd(cfg, IL0323_CMD_CDI, tmp, IL0323_CDI_REG_LENGTH)) {
        return -EIO;
    }

    tmp[0] = DT_INST_PROP(0, tcon);
    if (il0323_write_cmd(cfg, IL0323_CMD_TCON, tmp, 1)) {
        return -EIO;
    }

    /* Enable Auto Sequence */
    tmp[0] = IL0323_AUTO_PON_DRF_POF;
    if (il0323_write_cmd(cfg, IL0323_CMD_AUTO, tmp, 1)) {
        return -EIO;
    }

    return 0;
}

static int il0323_init(const struct device *dev) {
    const struct il0323_cfg *cfg = dev->config;

    if (!spi_is_ready_dt(&cfg->spi)) {
        LOG_ERR("SPI device not ready for IL0323");
        return -EIO;
    }

    if (!device_is_ready(cfg->reset.port)) {
        LOG_ERR("Could not get GPIO port for IL0323 reset");
        return -EIO;
    }

    gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);

    if (!device_is_ready(cfg->dc.port)) {
        LOG_ERR("Could not get GPIO port for IL0323 DC signal");
        return -EIO;
    }

    gpio_pin_configure_dt(&cfg->dc, GPIO_OUTPUT_INACTIVE);

    if (!device_is_ready(cfg->busy.port)) {
        LOG_ERR("Could not get GPIO port for IL0323 busy signal");
        return -EIO;
    }

    gpio_pin_configure_dt(&cfg->busy, GPIO_INPUT);

    return il0323_controller_init(dev);
}

static const struct il0323_cfg il0323_config = {
    .spi = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),
    .reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
    .busy = GPIO_DT_SPEC_INST_GET(0, busy_gpios),
    .dc = GPIO_DT_SPEC_INST_GET(0, dc_gpios),
};

static const struct display_driver_api il0323_driver_api = {
    .blanking_on = il0323_blanking_on,
    .blanking_off = il0323_blanking_off,
    .write = il0323_write,
    .read = il0323_read,
    .get_framebuffer = il0323_get_framebuffer,
    .set_brightness = il0323_set_brightness,
    .set_contrast = il0323_set_contrast,
    .get_capabilities = il0323_get_capabilities,
    .set_pixel_format = il0323_set_pixel_format,
    .set_orientation = il0323_set_orientation,
};

DEVICE_DT_INST_DEFINE(0, il0323_init, NULL, NULL, &il0323_config, POST_KERNEL,
                      CONFIG_APPLICATION_INIT_PRIORITY, &il0323_driver_api);
