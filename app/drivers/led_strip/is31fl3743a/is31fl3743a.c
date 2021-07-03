/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT issi_is31fl3743a

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/led_strip.h>
#include <logging/log.h>
#include <sys/math_extras.h>
#include <sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define IS31FL3743A_CS_PINS 18
#define IS31FL3743A_SW_PINS 11

#define IS31FL3743A_PSR (0xfd)
#define IS31FL3743A_PSWL (0xfe)
#define IS31FL3743A_PSWL_ENABLE (0xc5)
#define IS31FL3743A_PSWL_DISABLE (0x00)

#define IS31FL3743A_PAGE_PWM (0x00)
#define IS31FL3743A_PAGE_SCALING (0x01)
#define IS31FL3743A_PAGE_FUNCTION (0x02)

struct is31fl3743a_config {
    char *bus;
    int reg;
    char *label;
    char *sdb_port;
    gpio_pin_t sdb_pin;
    gpio_dt_flags_t sdb_flags;
    uint8_t *px_buffer;
    size_t px_buffer_size;
    uint8_t gcc;
    uint8_t sws;
    uint8_t sync;
    uint8_t *cs_map;
};

struct is31fl3743a_data {
    const struct device *i2c;
    const struct device *gpio;
};

static int is31fl3743a_reg_write(const struct device *dev, uint8_t addr, uint8_t value) {
    const struct is31fl3743a_data *data = dev->data;
    const struct is31fl3743a_config *config = dev->config;

    if (i2c_reg_write_byte(data->i2c, config->reg, addr, value)) {
        LOG_ERR("Failed writing value %x to register address %x on device %x.", value, addr,
                config->reg);
        return -EIO;
    }

    return 0;
}

static int is31fl3743a_reg_burst_write(const struct device *dev, uint8_t start_addr,
                                       const uint8_t *buffer, size_t num_bytes) {
    const struct is31fl3743a_data *data = dev->data;
    const struct is31fl3743a_config *config = dev->config;

    if (i2c_burst_write(data->i2c, config->reg, start_addr, buffer, num_bytes)) {
        LOG_ERR("Failed burst write with starting address %x", start_addr);
        return -EIO;
    }

    return 0;
}

static int is31fl3743a_set_page(const struct device *dev, uint8_t page_addr) {
    if (is31fl3743a_reg_write(dev, IS31FL3743A_PSWL, IS31FL3743A_PSWL_ENABLE)) {
        return -EIO;
    }

    if (is31fl3743a_reg_write(dev, IS31FL3743A_PSR, page_addr)) {
        return -EIO;
    }

    return 0;
}

static inline bool num_pixels_ok(const struct is31fl3743a_config *config, size_t num_pixels) {
    size_t num_bytes;

    const bool overflow = size_mul_overflow(num_pixels, 3, &num_bytes);

    return !overflow && (num_bytes <= config->px_buffer_size);
}

/*
 * Updates the RGB LED matrix using cs-order devicetree property
 * to assign correct R,G,B channels.
 */
static int is31fl3743a_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
                                        size_t num_pixels) {
    const struct is31fl3743a_config *config = dev->config;

    uint8_t *px_buffer = config->px_buffer;
    uint8_t *cs_map = config->cs_map;

    size_t sw_offset = 0;
    size_t cs = 0;

    if (!num_pixels_ok(config, num_pixels)) {
        return -ENOMEM;
    }

    for (size_t i = 0; i < num_pixels; ++i) {
        px_buffer[sw_offset + cs_map[cs++]] = pixels[i].r;
        px_buffer[sw_offset + cs_map[cs++]] = pixels[i].g;
        px_buffer[sw_offset + cs_map[cs++]] = pixels[i].b;

        if (IS31FL3743A_CS_PINS <= cs) {
            cs = 0;
            sw_offset += IS31FL3743A_CS_PINS;
        }
    }

    if (is31fl3743a_set_page(dev, IS31FL3743A_PAGE_PWM)) {
        LOG_ERR("Failed to set PWM page on %s", config->label);
        return -EIO;
    }

    return is31fl3743a_reg_burst_write(dev, 0x01, px_buffer, config->px_buffer_size);
}

/**
 * Updates individual LED channels without an RGB interpretation.
 */
static int is31fl3743a_strip_update_channels(const struct device *dev, uint8_t *channels,
                                             size_t num_channels) {
    const struct is31fl3743a_config *config = dev->config;

    if (config->px_buffer_size < num_channels) {
        return -ENOMEM;
    }

    return is31fl3743a_reg_burst_write(dev, 0x01, channels, num_channels);
}

/*
 * Initiates a driver instance for IS31FL3743A.
 *
 * SDB is pulled high to enable chip operation followed
 * by a reset to clear out all previous values.
 *
 * Function and scaling registers are then pre-configured based on devicetree settings.
 */
int static is31fl3743a_init(const struct device *dev) {
    struct is31fl3743a_data *data = dev->data;
    const struct is31fl3743a_config *config = dev->config;

    data->i2c = device_get_binding(config->bus);

    if (data->i2c == NULL) {
        LOG_ERR("I2C device %s not found", config->bus);
        return -ENODEV;
    }

    data->gpio = device_get_binding(config->sdb_port);

    if (data->gpio == NULL) {
        LOG_ERR("GPIO device %s not found", config->sdb_port);
        return -ENODEV;
    }

    gpio_pin_configure(data->gpio, config->sdb_pin, (GPIO_OUTPUT | config->sdb_flags));

    if (gpio_pin_set(data->gpio, config->sdb_pin, 1)) {
        LOG_ERR("SDB pin for %s cannot be pulled high", config->label);
        return -EIO;
    }

    // Set configuration registers
    if (is31fl3743a_set_page(dev, IS31FL3743A_PAGE_FUNCTION)) {
        LOG_ERR("Couldn't switch to function registers on %s", config->label);
        return -EIO;
    }

    is31fl3743a_reg_write(dev, 0x2f, 0xae); // Reset
    is31fl3743a_reg_write(
        dev, 0x00, (config->sws << 4) | (0x01 << 3) | 0x01); // SWS, H logic, Normal operation
    is31fl3743a_reg_write(dev, 0x01, config->gcc);           // Set GCC
    is31fl3743a_reg_write(dev, 0x24, 0x08);                // Thermal shutoff at 100*C, put into DT
    is31fl3743a_reg_write(dev, 0x25, (config->sync << 6)); // Set SYNC setting

    // Set scaling registers, default to 0xff
    if (is31fl3743a_set_page(dev, IS31FL3743A_PAGE_SCALING)) {
        LOG_ERR("Couldn't switch to scaling registers on %s", config->label);
    }

    uint8_t scaling_buffer[config->px_buffer_size];

    for (size_t i = 0; i < config->px_buffer_size; ++i) {
        px_buffer[i] = 0xff;
    }

    is31fl3743a_reg_burst_write(dev, 0x01, scaling_buffer, config->px_buffer_size);

    return 0;
}

static const struct led_strip_driver_api is31fl3743a_api = {
    .update_rgb = is31fl3743a_strip_update_rgb,
    .update_channels = is31fl3743a_strip_update_channels,
};

#define IS31FL3743A_BUFFER_SIZE(idx)                                                               \
    IS31FL3743A_CS_PINS *(IS31FL3743A_SW_PINS - DT_INST_PROP(idx, sw_setting))

#define IS31FL3743A_GCC(idx)                                                                       \
    (DT_INST_PROP(idx, riset) * DT_INST_PROP(idx, led_max_current) * 256 * 256) / (343 * 255)

#define IS31FL3743A_DEVICE(idx)                                                                    \
                                                                                                   \
    static struct is31fl3743a_data is31fl3743a_##idx##_data;                                       \
                                                                                                   \
    static uint8_t is31fl3743a_##idx##_px_buffer[IS31FL3743A_BUFFER_SIZE(idx)];                    \
                                                                                                   \
    static uint8_t is31fl3743a_##idx##_cs_map[] = DT_INST_PROP(idx, cs_order);                     \
                                                                                                   \
    static const struct is31fl3743a_config is31fl3743a_##idx##_config = {                          \
        .bus = DT_INST_BUS_LABEL(idx),                                                             \
        .reg = DT_INST_REG_ADDR(idx),                                                              \
        .label = DT_INST_LABEL(idx),                                                               \
        .sdb_port = DT_INST_GPIO_LABEL(idx, sdb_gpios),                                            \
        .sdb_pin = DT_INST_GPIO_PIN(idx, sdb_gpios),                                               \
        .sdb_flags = DT_INST_GPIO_FLAGS(idx, sdb_gpios),                                           \
        .px_buffer = is31fl3743a_##idx##_px_buffer,                                                \
        .px_buffer_size = IS31FL3743A_BUFFER_SIZE(idx),                                            \
        .gcc = IS31FL3743A_GCC(idx),                                                               \
        .sws = DT_INST_PROP(idx, sw_setting),                                                      \
        .sync = DT_INST_PROP(idx, sync),                                                           \
        .cs_map = is31fl3743a_##idx##_cs_map,                                                      \
    };                                                                                             \
                                                                                                   \
    DEVICE_AND_API_INIT(is31fl3743a_##idx, DT_INST_LABEL(idx), &is31fl3743a_init,                  \
                        &is31fl3743a_##idx##_data, &is31fl3743a_##idx##_config, POST_KERNEL,       \
                        CONFIG_LED_STRIP_INIT_PRIORITY, &is31fl3743a_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3743A_DEVICE);
