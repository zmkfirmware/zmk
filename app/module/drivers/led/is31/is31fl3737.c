/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_is31fl3737

#include <drivers/led/is31fl3737.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(is31fl3737, CONFIG_LED_LOG_LEVEL);

#define IS31FL3737_CMD_SEL_REG 0xFD
#define IS31FL3737_CMD_LOCK_REG 0xFE
#define IS31FL3737_CMD_LOCK_UNLOCK 0xC5

#define IS31FL3737_PAGE_LED_ENABLE 0x00
#define IS31FL3737_PAGE_PWM 0x01
#define IS31FL3737_PAGE_ABM 0x02
#define IS31FL3737_PAGE_FUNCTION 0x03

#define IS31FL3737_CONFIG_REG 0x00
#define IS31FL3737_GLOBAL_CURRENT_REG 0x01
#define IS31FL3737_ABM_BANK1_REG 0x02
#define IS31FL3737_ABM_UPDATE_REG 0x0E

#define IS31FL3737_CONFIG_NORMAL_EN BIT(0)
#define IS31FL3737_CONFIG_ABM_EN BIT(1)

#define IS31FL3737_LED_ENABLE_REG_COUNT 24
#define IS31FL3737_PG12_MAX_REG 0xBD
#define IS31FL3737_PG12_REG_COUNT (IS31FL3737_PG12_MAX_REG + 1U)
#define IS31FL3737_DENSE_CHANNELS_PER_BANK 12U
#define IS31FL3737_RAW_CHANNEL_STRIDE 16U
#define IS31FL3737_DENSE_CHANNEL_COUNT                                                      \
    (((IS31FL3737_PG12_MAX_REG / IS31FL3737_RAW_CHANNEL_STRIDE) + 1U) *                     \
     IS31FL3737_DENSE_CHANNELS_PER_BANK)
#define IS31FL3737_ABM_BANK_COUNT 3
#define IS31FL3737_ABM_BANK_REG_COUNT 4
#define IS31FL3737_MAX_BRIGHTNESS 100

struct is31fl3737_config {
    struct i2c_dt_spec bus;
    struct gpio_dt_spec sdb;
    uint8_t current_limit;
};

struct is31fl3737_data {
    struct k_mutex lock;
    uint8_t selected_page;
    uint8_t conf_reg;
    uint8_t scratch_buf[IS31FL3737_PG12_REG_COUNT + 1];
};

static inline void is31fl3737_lock(struct is31fl3737_data *data) { k_mutex_lock(&data->lock, K_FOREVER); }

static inline void is31fl3737_unlock(struct is31fl3737_data *data) { k_mutex_unlock(&data->lock); }

static uint8_t is31fl3737_dense_to_raw_channel(uint32_t channel) {
    uint32_t bank = channel / IS31FL3737_DENSE_CHANNELS_PER_BANK;
    uint32_t offset = channel % IS31FL3737_DENSE_CHANNELS_PER_BANK;

    return (uint8_t)((bank * IS31FL3737_RAW_CHANNEL_STRIDE) + offset);
}

static int is31fl3737_validate_page_range(uint8_t page, uint8_t start_reg, size_t len) {
    size_t end_reg;

    if (len == 0U) {
        return -EINVAL;
    }

    if (page != IS31FL3737_PAGE_PWM && page != IS31FL3737_PAGE_ABM) {
        return 0;
    }

    end_reg = (size_t)start_reg + len - 1U;
    return end_reg <= IS31FL3737_PG12_MAX_REG ? 0 : -EINVAL;
}

static int is31fl3737_select_page(const struct device *dev, uint8_t page) {
    const struct is31fl3737_config *config = dev->config;
    struct is31fl3737_data *data = dev->data;
    int ret;

    if (data->selected_page == page) {
        return 0;
    }

    ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3737_CMD_LOCK_REG, IS31FL3737_CMD_LOCK_UNLOCK);
    if (ret < 0) {
        data->selected_page = UINT8_MAX;
        LOG_ERR("Could not unlock page selection register");
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3737_CMD_SEL_REG, page);
    if (ret < 0) {
        data->selected_page = UINT8_MAX;
        LOG_ERR("Could not select page %u", page);
        return ret;
    }

    data->selected_page = page;
    return 0;
}

static int is31fl3737_write_page_buffer(const struct device *dev, uint8_t page, uint8_t start_reg,
                                        const uint8_t *buf, size_t len) {
    const struct is31fl3737_config *config = dev->config;
    struct is31fl3737_data *data = dev->data;
    int ret;

    if (buf == NULL || len == 0U || (len + 1U) > ARRAY_SIZE(data->scratch_buf)) {
        return -EINVAL;
    }

    ret = is31fl3737_validate_page_range(page, start_reg, len);
    if (ret < 0) {
        return ret;
    }

    ret = is31fl3737_select_page(dev, page);
    if (ret < 0) {
        return ret;
    }

    data->scratch_buf[0] = start_reg;
    memcpy(&data->scratch_buf[1], buf, len);

    return i2c_write_dt(&config->bus, data->scratch_buf, len + 1U);
}

static int is31fl3737_fill_page_buffer(const struct device *dev, uint8_t page, uint8_t start_reg,
                                       uint8_t value, size_t len) {
    const struct is31fl3737_config *config = dev->config;
    struct is31fl3737_data *data = dev->data;
    int ret;

    if (len == 0U || (len + 1U) > ARRAY_SIZE(data->scratch_buf)) {
        return -EINVAL;
    }

    ret = is31fl3737_validate_page_range(page, start_reg, len);
    if (ret < 0) {
        return ret;
    }

    ret = is31fl3737_select_page(dev, page);
    if (ret < 0) {
        return ret;
    }

    data->scratch_buf[0] = start_reg;
    memset(&data->scratch_buf[1], value, len);
    return i2c_write_dt(&config->bus, data->scratch_buf, len + 1U);
}

static int is31fl3737_write_conf_reg(const struct device *dev) {
    const struct is31fl3737_config *config = dev->config;
    struct is31fl3737_data *data = dev->data;
    int ret = is31fl3737_select_page(dev, IS31FL3737_PAGE_FUNCTION);

    if (ret < 0) {
        return ret;
    }

    return i2c_reg_write_byte_dt(&config->bus, IS31FL3737_CONFIG_REG, data->conf_reg);
}

static int is31fl3737_led_set_brightness_locked(const struct device *dev, uint32_t led, uint8_t value) {
    const struct is31fl3737_config *config = dev->config;
    uint8_t raw_channel;
    uint8_t pwm_value;
    int ret;

    if (led >= IS31FL3737_DENSE_CHANNEL_COUNT || value > IS31FL3737_MAX_BRIGHTNESS) {
        return -EINVAL;
    }

    raw_channel = is31fl3737_dense_to_raw_channel(led);
    pwm_value = (uint8_t)(((uint32_t)value * 255U) / IS31FL3737_MAX_BRIGHTNESS);

    ret = is31fl3737_select_page(dev, IS31FL3737_PAGE_PWM);
    if (ret < 0) {
        return ret;
    }

    return i2c_reg_write_byte_dt(&config->bus, raw_channel, pwm_value);
}

static int is31fl3737_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value) {
    struct is31fl3737_data *data = dev->data;
    int ret;

    is31fl3737_lock(data);
    ret = is31fl3737_led_set_brightness_locked(dev, led, value);
    is31fl3737_unlock(data);

    return ret;
}

static int is31fl3737_led_on(const struct device *dev, uint32_t led) {
    struct is31fl3737_data *data = dev->data;
    int ret;

    is31fl3737_lock(data);
    ret = is31fl3737_led_set_brightness_locked(dev, led, IS31FL3737_MAX_BRIGHTNESS);
    is31fl3737_unlock(data);

    return ret;
}

static int is31fl3737_led_off(const struct device *dev, uint32_t led) {
    struct is31fl3737_data *data = dev->data;
    int ret;

    is31fl3737_lock(data);
    ret = is31fl3737_led_set_brightness_locked(dev, led, 0U);
    is31fl3737_unlock(data);

    return ret;
}

static int is31fl3737_led_write_channels(const struct device *dev, uint32_t start_channel,
                                         uint32_t num_channels, const uint8_t *buf) {
    struct is31fl3737_data *data = dev->data;
    uint32_t cursor = start_channel;
    int ret = 0;

    if (buf == NULL && num_channels > 0U) {
        return -EINVAL;
    }

    if ((start_channel + num_channels) > IS31FL3737_DENSE_CHANNEL_COUNT) {
        return -EINVAL;
    }

    if (num_channels == 0U) {
        return 0;
    }

    is31fl3737_lock(data);
    while (num_channels > 0U) {
        uint32_t bank_offset = cursor % IS31FL3737_DENSE_CHANNELS_PER_BANK;
        uint32_t chunk_len = MIN(num_channels, IS31FL3737_DENSE_CHANNELS_PER_BANK - bank_offset);

        ret = is31fl3737_write_page_buffer(dev, IS31FL3737_PAGE_PWM,
                                           is31fl3737_dense_to_raw_channel(cursor), buf, chunk_len);
        if (ret < 0) {
            break;
        }

        cursor += chunk_len;
        buf += chunk_len;
        num_channels -= chunk_len;
    }
    is31fl3737_unlock(data);

    return ret;
}

static int is31fl3737_program_defaults_locked(const struct device *dev) {
    const struct is31fl3737_config *config = dev->config;
    struct is31fl3737_data *data = dev->data;
    int ret;

    data->selected_page = UINT8_MAX;

    ret = is31fl3737_fill_page_buffer(dev, IS31FL3737_PAGE_LED_ENABLE, 0x00, 0xFF,
                                      IS31FL3737_LED_ENABLE_REG_COUNT);
    if (ret < 0) {
        return ret;
    }

    ret = is31fl3737_select_page(dev, IS31FL3737_PAGE_FUNCTION);
    if (ret < 0) {
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3737_GLOBAL_CURRENT_REG, config->current_limit);
    if (ret < 0) {
        return ret;
    }

    /* Keep outputs blank until every PWM/ABM register has been initialized. */
    data->conf_reg = 0U;
    ret = is31fl3737_write_conf_reg(dev);
    if (ret < 0) {
        return ret;
    }

    ret = is31fl3737_fill_page_buffer(dev, IS31FL3737_PAGE_ABM, 0x00, 0x00, IS31FL3737_PG12_REG_COUNT);
    if (ret < 0) {
        return ret;
    }

    ret = is31fl3737_fill_page_buffer(dev, IS31FL3737_PAGE_PWM, 0x00, 0x00,
                                      IS31FL3737_PG12_REG_COUNT);
    if (ret < 0) {
        return ret;
    }

    data->conf_reg = IS31FL3737_CONFIG_NORMAL_EN;
    return is31fl3737_write_conf_reg(dev);
}

int is31fl3737_resume(const struct device *dev) {
    struct is31fl3737_data *data = dev->data;

    is31fl3737_lock(data);
    int ret = is31fl3737_program_defaults_locked(dev);
    if (ret != 0) {
        data->selected_page = UINT8_MAX;
    }
    is31fl3737_unlock(data);
    return ret;
}

static int is31fl3737_init(const struct device *dev) {
    const struct is31fl3737_config *config = dev->config;
    struct is31fl3737_data *data = dev->data;
    int ret;

    k_mutex_init(&data->lock);

    if (!i2c_is_ready_dt(&config->bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    if (config->sdb.port != NULL) {
        if (!gpio_is_ready_dt(&config->sdb)) {
            LOG_ERR("GPIO SDB pin not ready");
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&config->sdb, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            return ret;
        }
    }

    is31fl3737_lock(data);
    ret = is31fl3737_program_defaults_locked(dev);
    is31fl3737_unlock(data);

    if (ret != 0 && config->sdb.port != NULL) {
        int off_err = gpio_pin_set_dt(&config->sdb, 0);
        if (off_err != 0) {
            LOG_ERR("Initialization failed (%d), SDB shutdown failed (%d)", ret, off_err);
        }
    }
    return ret;
}

int is31fl3737_blank(const struct device *dev, bool blank_en) {
    struct is31fl3737_data *data = dev->data;
    int ret;

    is31fl3737_lock(data);

    if (blank_en) {
        data->conf_reg &= ~IS31FL3737_CONFIG_NORMAL_EN;
    } else {
        data->conf_reg |= IS31FL3737_CONFIG_NORMAL_EN;
    }

    ret = is31fl3737_write_conf_reg(dev);
    is31fl3737_unlock(data);

    return ret;
}

int is31fl3737_current_limit(const struct device *dev, uint8_t limit) {
    const struct is31fl3737_config *config = dev->config;
    struct is31fl3737_data *data = dev->data;
    int ret;

    is31fl3737_lock(data);
    ret = is31fl3737_select_page(dev, IS31FL3737_PAGE_FUNCTION);
    if (ret < 0) {
        is31fl3737_unlock(data);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3737_GLOBAL_CURRENT_REG, limit);
    is31fl3737_unlock(data);

    return ret;
}

int is31fl3737_write_abm_modes(const struct device *dev, uint32_t start_channel,
                               uint32_t num_channels, const uint8_t *buf) {
    struct is31fl3737_data *data = dev->data;
    uint32_t cursor = start_channel;
    int ret = 0;

    if (buf == NULL && num_channels > 0U) {
        return -EINVAL;
    }

    if ((start_channel + num_channels) > IS31FL3737_DENSE_CHANNEL_COUNT) {
        return -EINVAL;
    }

    if (num_channels == 0U) {
        return 0;
    }

    is31fl3737_lock(data);
    while (num_channels > 0U) {
        uint32_t bank_offset = cursor % IS31FL3737_DENSE_CHANNELS_PER_BANK;
        uint32_t chunk_len = MIN(num_channels, IS31FL3737_DENSE_CHANNELS_PER_BANK - bank_offset);

        ret = is31fl3737_write_page_buffer(dev, IS31FL3737_PAGE_ABM,
                                           is31fl3737_dense_to_raw_channel(cursor), buf, chunk_len);
        if (ret < 0) {
            break;
        }

        cursor += chunk_len;
        buf += chunk_len;
        num_channels -= chunk_len;
    }
    is31fl3737_unlock(data);

    return ret;
}

int is31fl3737_configure_abm_bank(const struct device *dev, uint8_t bank, const uint8_t regs[4]) {
    const struct is31fl3737_config *config = dev->config;
    struct is31fl3737_data *data = dev->data;
    uint8_t start_reg;
    int ret;

    if (bank == 0U || bank > IS31FL3737_ABM_BANK_COUNT || regs == NULL) {
        return -EINVAL;
    }

    start_reg = IS31FL3737_ABM_BANK1_REG + ((bank - 1U) * IS31FL3737_ABM_BANK_REG_COUNT);
    is31fl3737_lock(data);
    ret = is31fl3737_write_page_buffer(dev, IS31FL3737_PAGE_FUNCTION, start_reg, regs,
                                       IS31FL3737_ABM_BANK_REG_COUNT);
    if (ret < 0) {
        is31fl3737_unlock(data);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3737_ABM_UPDATE_REG, 0x00);
    is31fl3737_unlock(data);

    return ret;
}

int is31fl3737_enable_abm(const struct device *dev, bool enable) {
    struct is31fl3737_data *data = dev->data;
    int ret;

    is31fl3737_lock(data);

    if (enable) {
        data->conf_reg |= IS31FL3737_CONFIG_ABM_EN;
    } else {
        data->conf_reg &= ~IS31FL3737_CONFIG_ABM_EN;
    }

    ret = is31fl3737_write_conf_reg(dev);
    is31fl3737_unlock(data);

    return ret;
}

static DEVICE_API(led, is31fl3737_api) = {
    .on = is31fl3737_led_on,
    .off = is31fl3737_led_off,
    .set_brightness = is31fl3737_led_set_brightness,
    .write_channels = is31fl3737_led_write_channels,
};

#define IS31FL3737_DEVICE(inst)                                                                   \
    static const struct is31fl3737_config is31fl3737_config_##inst = {                           \
        .bus = I2C_DT_SPEC_INST_GET(inst),                                                        \
        .sdb = GPIO_DT_SPEC_INST_GET_OR(inst, sdb_gpios, {}),                                    \
        .current_limit = DT_INST_PROP(inst, current_limit),                                      \
    };                                                                                            \
                                                                                                  \
    static struct is31fl3737_data is31fl3737_data_##inst = {                                     \
        .selected_page = UINT8_MAX,                                                               \
    };                                                                                            \
                                                                                                  \
    DEVICE_DT_INST_DEFINE(inst, &is31fl3737_init, NULL, &is31fl3737_data_##inst,                \
                          &is31fl3737_config_##inst, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,      \
                          &is31fl3737_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3737_DEVICE)
