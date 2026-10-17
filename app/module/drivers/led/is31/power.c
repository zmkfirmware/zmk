/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/led/is31_power.h>

#include <drivers/led/is31fl3737.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/drivers/led/is31fl3733.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define IS31_CMD_SEL_REG 0xFD
#define IS31_CMD_LOCK_REG 0xFE
#define IS31_CMD_LOCK_UNLOCK 0xC5

#define IS31_PAGE_LED_ENABLE 0x00
#define IS31_PAGE_PWM 0x01
#define IS31_PAGE_ABM 0x02
#define IS31_PAGE_FUNCTION 0x03

#define IS31FL3733_CONFIG_REG 0x00
#define IS31FL3733_GLOBAL_CURRENT_REG 0x01
#define IS31FL3733_RESET_REG 0x11
#define IS31FL3733_CONFIG_SSD_MASK BIT(0)
#define IS31FL3733_CONFIG_SYNC_SHIFT 6
#define IS31FL3733_LED_ENABLE_REG_COUNT (192 / 8)

static int zmk_is31_validate_bus(const struct zmk_is31_power_config *config) {
    if (config == NULL || config->dev == NULL || config->chip == ZMK_IS31_CHIP_NONE) {
        return -EINVAL;
    }

    if (!device_is_ready(config->dev) || !i2c_is_ready_dt(&config->bus)) {
        return -ENODEV;
    }

    if (config->sdb.port != NULL && !gpio_is_ready_dt(&config->sdb)) {
        return -ENODEV;
    }

    return 0;
}

static int zmk_is31_set_sdb(const struct zmk_is31_power_config *config, bool enable) {
    if (config->sdb.port == NULL) {
        return 0;
    }

    return gpio_pin_set_dt(&config->sdb, enable ? 1 : 0);
}

#if IS_ENABLED(CONFIG_IS31FL3733)
static int zmk_is31_select_page(const struct i2c_dt_spec *bus, uint8_t page) {
    int ret = i2c_reg_write_byte_dt(bus, IS31_CMD_LOCK_REG, IS31_CMD_LOCK_UNLOCK);

    if (ret < 0) {
        return ret;
    }

    return i2c_reg_write_byte_dt(bus, IS31_CMD_SEL_REG, page);
}

static int zmk_is31fl3733_program_defaults(const struct zmk_is31_power_config *config) {
    uint8_t led_enable[IS31FL3733_LED_ENABLE_REG_COUNT + 1U];
    uint8_t dummy;
    uint8_t conf_reg = (config->sync << IS31FL3733_CONFIG_SYNC_SHIFT) | IS31FL3733_CONFIG_SSD_MASK;
    int ret;

    ret = zmk_is31_select_page(&config->bus, IS31_PAGE_FUNCTION);
    if (ret < 0) {
        return ret;
    }

    ret = i2c_reg_read_byte_dt(&config->bus, IS31FL3733_RESET_REG, &dummy);
    if (ret < 0) {
        return ret;
    }

    ret = zmk_is31_select_page(&config->bus, IS31_PAGE_FUNCTION);
    if (ret < 0) {
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3733_GLOBAL_CURRENT_REG, config->current_limit);
    if (ret < 0) {
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3733_CONFIG_REG, conf_reg);
    if (ret < 0) {
        return ret;
    }

    led_enable[0] = 0x00;
    memset(&led_enable[1], 0xFF, IS31FL3733_LED_ENABLE_REG_COUNT);

    ret = zmk_is31_select_page(&config->bus, IS31_PAGE_LED_ENABLE);
    if (ret < 0) {
        return ret;
    }

    ret = i2c_write_dt(&config->bus, led_enable, sizeof(led_enable));
    if (ret < 0) {
        return ret;
    }

    return zmk_is31_select_page(&config->bus, IS31_PAGE_FUNCTION);
}
#endif

int zmk_is31_enter_hard_off(const struct zmk_is31_power_config *config) {
    if (config == NULL || config->dev == NULL || config->chip == ZMK_IS31_CHIP_NONE) {
        return -EINVAL;
    }

    int blank_err = zmk_is31_blank(config, true);
    if (config->sdb.port == NULL) {
        return blank_err;
    }

    /* SDB is the fallback even when the LED device or I2C bus is not ready. */
    int sdb_err = gpio_is_ready_dt(&config->sdb) ? zmk_is31_set_sdb(config, false) : -ENODEV;
    if (blank_err != 0) {
        LOG_WRN("IS31 software shutdown failed (%d); SDB shutdown result: %d", blank_err, sdb_err);
    }
    return sdb_err;
}

int zmk_is31_resume_from_hard_off(const struct zmk_is31_power_config *config) {
    int ret = zmk_is31_validate_bus(config);

    if (ret < 0) {
        goto failed;
    }

    ret = zmk_is31_set_sdb(config, true);
    if (ret == 0) {
        switch (config->chip) {
#if IS_ENABLED(CONFIG_IS31FL3733)
        case ZMK_IS31_CHIP_IS31FL3733:
            ret = zmk_is31fl3733_program_defaults(config);
            if (ret == 0) {
                ret = is31fl3733_blank(config->dev, false);
            }
            break;
#endif
#if IS_ENABLED(CONFIG_ZMK_IS31FL3737)
        case ZMK_IS31_CHIP_IS31FL3737:
            ret = is31fl3737_resume(config->dev);
            break;
#endif
        default:
            ret = -ENOTSUP;
            break;
        }
    }

failed:
    if (ret != 0) {
        /* Do not leave an incompletely initialized device drawing current. */
        int off_err = zmk_is31_enter_hard_off(config);
        if (off_err != 0) {
            LOG_ERR("IS31 resume failed (%d), shutdown fallback also failed (%d)", ret, off_err);
        }
    }
    return ret;
}

int zmk_is31_blank(const struct zmk_is31_power_config *config, bool blank_en) {
    int ret = zmk_is31_validate_bus(config);

    if (ret < 0) {
        return ret;
    }

    switch (config->chip) {
#if IS_ENABLED(CONFIG_IS31FL3733)
    case ZMK_IS31_CHIP_IS31FL3733:
        return is31fl3733_blank(config->dev, blank_en);
#endif
#if IS_ENABLED(CONFIG_ZMK_IS31FL3737)
    case ZMK_IS31_CHIP_IS31FL3737:
        return is31fl3737_blank(config->dev, blank_en);
#endif
    default:
        return -ENOTSUP;
    }
}
