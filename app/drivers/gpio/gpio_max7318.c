/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT maxim_max7318

/**
 * @file Driver for MAX7318 I2C-based GPIO driver.
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/ext_power.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>

LOG_MODULE_REGISTER(gpio_max7318);

// Register definitions
#define REG_INPUT_PORTA 0x00
#define REG_INPUT_PORTB 0x01
#define REG_OUTPUT_PORTA 0x02
#define REG_OUTPUT_PORTB 0x03
#define REG_IPOL_PORTA 0x04
#define REG_IPOL_PORTB 0x05
#define REG_CONFIG_PORTA 0x06
#define REG_CONFIG_PORTB 0x07

// Configuration data
struct max7318_config {
    struct gpio_driver_config common;

    struct i2c_dt_spec i2c_bus;
    uint8_t ngpios;
};

// Runtime driver data
struct max7318_drv_data {
    // gpio_driver_data needs to be first
    struct gpio_driver_config data;

    struct k_sem lock;

    struct {
        uint16_t ipol;
        uint16_t config;
        uint16_t output;
    } reg_cache;
};

/**
 * @brief Read the value of two consecutive registers
 *
 * Read two consecutive bytes from the register at address `reg` and `reg + 1`,
 * typically reading from registers for port 0 and 1 simultaneously.
 *
 * @param dev   The max7318 device.
 * @param reg   Register to read (the PORT0 of the pair of registers).
 * @param buf   Buffer to read data into.
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_registers(const struct device *dev, uint8_t reg, uint16_t *buf) {
    const struct max7318_config *config = dev->config;

    uint16_t data = 0;
    int ret = i2c_burst_read_dt(&config->i2c_bus, reg, (uint8_t *)&data, sizeof(data));
    if (ret) {
        LOG_DBG("i2c_write_read FAIL %d\n", ret);
        return ret;
    }

    *buf = sys_le16_to_cpu(data);

    LOG_DBG("max7318: read: reg[0x%X] = 0x%X, reg[0x%X] = 0x%X", reg, (*buf & 0xFF), (reg + 1),
            (*buf >> 8));

    return 0;
}

/**
 * @brief Write the value of two consecutive registers
 *
 * Write two consecutive bytes from the register at address `reg` and `reg + 1`,
 * typically to registers for port 0 and 1 simultaneously.
 *
 * @param dev   The max7318 device.
 * @param reg   Register to write (usually the register for PORT0).
 * @param value The value to write
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_registers(const struct device *dev, uint8_t reg, uint16_t value) {
    const struct max7318_config *config = dev->config;

    LOG_DBG("max7318: write: reg[0x%X] = 0x%X, reg[0x%X] = 0x%X", reg, (value & 0xFF), (reg + 1),
            (value >> 8));

    uint16_t data = sys_cpu_to_le16(value);

    return i2c_burst_write_dt(&config->i2c_bus, reg, (uint8_t *)&data, sizeof(data));
}

/**
 * @brief Setup the pin direction (input or output)
 *
 * @param dev   The max7318 device.
 * @param pin   The pin number.
 * @param flags Flags of pin or port.
 *
 * @return 0 if successful, failed otherwise
 */
static int set_pin_direction(const struct device *dev, uint32_t pin, int flags) {
    struct max7318_drv_data *const drv_data = (struct max7318_drv_data *const)dev->data;
    uint16_t *dir = &drv_data->reg_cache.config;
    uint16_t *output = &drv_data->reg_cache.output;

    /*
        The output register is 1=high, 0=low; the direction (config) register
        is 1=input, 0=output.
    */
    if ((flags & GPIO_OUTPUT) != 0U) {
        if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
            *output |= BIT(pin);
        } else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
            *output &= ~BIT(pin);
        }
        *dir &= ~BIT(pin);
    } else {
        *dir |= BIT(pin);
    }

    int ret = write_registers(dev, REG_OUTPUT_PORTA, *output);
    if (ret != 0) {
        return ret;
    }

    return write_registers(dev, REG_CONFIG_PORTA, *dir);
}

/**
 * @brief Setup the pin pull up/pull down status. This function doesn't actually set any
 *        registers, since the max7318 only supports a pullup, and it can't be controlled.
 *
 * @param dev   The max7318 device.
 * @param pin   The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int set_pin_pull_direction(const struct device *dev, uint32_t pin, int flags) {
    // actually, this chip only supports pull-up, and it can't be disabled.
    // so, if we try to set anything else, return enotsup; we don't actually
    // need to set any registers.
    if ((flags & GPIO_PULL_DOWN) != 0U) {
        return -ENOTSUP;
    }

    return 0;
}

static int max7318_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags) {
    struct max7318_drv_data *const drv_data = (struct max7318_drv_data *const)dev->data;

    /* Can't do I2C bus operations from an ISR */
    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    k_sem_take(&drv_data->lock, K_FOREVER);

    int ret = 0;
    if ((flags & GPIO_OPEN_DRAIN) != 0U) {
        ret = -ENOTSUP;
        goto done;
    };

    ret = set_pin_direction(dev, pin, flags);
    if (ret != 0) {
        LOG_ERR("error setting pin direction (%d)", ret);
        goto done;
    }

    ret = set_pin_pull_direction(dev, pin, flags);
    if (ret != 0) {
        LOG_ERR("error setting pin pull up/down (%d)", ret);
        goto done;
    }

done:
    k_sem_give(&drv_data->lock);
    return ret;
}

static int max7318_port_get_raw(const struct device *dev, uint32_t *value) {
    struct max7318_drv_data *const drv_data = (struct max7318_drv_data *const)dev->data;

    /* Can't do I2C bus operations from an ISR */
    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    k_sem_take(&drv_data->lock, K_FOREVER);

    uint16_t buf = 0;
    int ret = read_registers(dev, REG_INPUT_PORTA, &buf);
    if (ret != 0) {
        goto done;
    }

    *value = buf;

done:
    k_sem_give(&drv_data->lock);
    return ret;
}

static int max7318_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value) {
    struct max7318_drv_data *const drv_data = (struct max7318_drv_data *const)dev->data;

    /* Can't do I2C bus operations from an ISR */
    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    k_sem_take(&drv_data->lock, K_FOREVER);

    uint16_t buf = drv_data->reg_cache.output;
    buf = (buf & ~mask) | (mask & value);

    int ret = write_registers(dev, REG_OUTPUT_PORTA, buf);
    if (ret == 0) {
        drv_data->reg_cache.output = buf;
    }

    k_sem_give(&drv_data->lock);
    return ret;
}

static int max7318_port_set_bits_raw(const struct device *dev, uint32_t mask) {
    return max7318_port_set_masked_raw(dev, mask, mask);
}

static int max7318_port_clear_bits_raw(const struct device *dev, uint32_t mask) {
    return max7318_port_set_masked_raw(dev, mask, 0);
}

static int max7318_port_toggle_bits(const struct device *dev, uint32_t mask) {
    struct max7318_drv_data *const drv_data = (struct max7318_drv_data *const)dev->data;

    /* Can't do I2C bus operations from an ISR */
    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    k_sem_take(&drv_data->lock, K_FOREVER);

    uint16_t buf = drv_data->reg_cache.output;
    buf ^= mask;

    int ret = write_registers(dev, REG_OUTPUT_PORTA, buf);
    if (ret == 0) {
        drv_data->reg_cache.output = buf;
    }

    k_sem_give(&drv_data->lock);
    return ret;
}

static int max7318_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
                                           enum gpio_int_mode mode, enum gpio_int_trig trig) {
    return -ENOTSUP;
}

static const struct gpio_driver_api api_table = {
    .pin_configure = max7318_config,
    .port_get_raw = max7318_port_get_raw,
    .port_set_masked_raw = max7318_port_set_masked_raw,
    .port_set_bits_raw = max7318_port_set_bits_raw,
    .port_clear_bits_raw = max7318_port_clear_bits_raw,
    .port_toggle_bits = max7318_port_toggle_bits,
    .pin_interrupt_configure = max7318_pin_interrupt_configure,
};

/**
 * @brief Initialisation function of MAX7318
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int max7318_init(const struct device *dev) {
    const struct max7318_config *const config = dev->config;
    struct max7318_drv_data *const drv_data = (struct max7318_drv_data *const)dev->data;

    if (!device_is_ready(config->i2c_bus.bus)) {
        LOG_WRN("i2c bus not ready!");
        return -EINVAL;
    }

    LOG_INF("device initialised at 0x%x", config->i2c_bus.addr);

    k_sem_init(&drv_data->lock, 1, 1);
    return 0;
}

#define GPIO_PORT_PIN_MASK_FROM_NGPIOS(ngpios) ((gpio_port_pins_t)(((uint64_t)1 << (ngpios)) - 1U))

#define GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)                                                      \
    GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_PROP(inst, ngpios))

#define MAX7318_INIT(inst)                                                                         \
    static struct max7318_config max7318_##inst##_config = {                                       \
        .common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)},                        \
        .i2c_bus = I2C_DT_SPEC_INST_GET(inst)};                                                    \
                                                                                                   \
    static struct max7318_drv_data max7318_##inst##_drvdata = {                                    \
        /* Default for registers according to datasheet */                                         \
        .reg_cache.ipol = 0x0,                                                                     \
        .reg_cache.config = 0xFFFF,                                                                \
        .reg_cache.output = 0xFFFF,                                                                \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(inst, max7318_init, NULL, &max7318_##inst##_drvdata,                     \
                          &max7318_##inst##_config, POST_KERNEL,                                   \
                          CONFIG_GPIO_MAX7318_INIT_PRIORITY, &api_table);

DT_INST_FOREACH_STATUS_OKAY(MAX7318_INIT)
