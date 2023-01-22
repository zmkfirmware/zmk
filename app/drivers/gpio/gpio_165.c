/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_gpio_165

/**
 * @file Driver for 165 parallel-input shift register, SPI-based GPIO driver.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL

#define LOAD_GPIOS_LEN(n) DT_INST_PROP_LEN(n, load_gpios)
#define LOAD_GPIO_CFG_INIT(idx, inst_idx)                                                          \
    GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst_idx), load_gpios, idx)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_165);

/** Configuration data */
struct reg_165_config {
    /* gpio_driver_data needs to be first */
    struct gpio_driver_config common;

    struct spi_dt_spec bus;

    uint8_t ngpios;

    const struct gpio_dt_spec *load_gpios;
    size_t load_gpios_len;
};

/** Runtime driver data */
struct reg_165_drv_data {
    /* gpio_driver_data needs to be first */
    struct gpio_driver_config data;

    struct k_sem lock;
};

static int reg_165_read_registers(const struct device *dev, uint32_t *value) {
    const struct reg_165_config *config = dev->config;
    int ret = 0;

    uint8_t nread = config->ngpios / 8;
    uint32_t reg_data = 0;

    /* Allow a sequence of 1-4 registers in sequence, lowest byte is for the first in the chain */
    const struct spi_buf rx_buf[1] = {{
        .buf = ((uint8_t *)&reg_data) + (4 - nread),
        .len = nread,
    }};

    const struct spi_buf_set rx = {
        .buffers = rx_buf,
        .count = ARRAY_SIZE(rx_buf),
    };

    for (int i = 0; i < config->load_gpios_len; i++) {
        int err = gpio_pin_set_dt(&config->load_gpios[i], 0);
        if (err) {
            LOG_ERR("Unable to set pin %u on %s to idle value '0'", config->load_gpios[i].pin,
                    config->load_gpios[i].port->name);
            return err;
        }
    }

    for (int i = 0; i < config->load_gpios_len; i++) {
        int err = gpio_pin_set_dt(&config->load_gpios[i], 1);
        if (err) {
            LOG_ERR("Unable to set pin %u on %s to idle value '1'", config->load_gpios[i].pin,
                    config->load_gpios[i].port->name);
            return err;
        }
    }

    ret = spi_read_dt(&config->bus, &rx);

    *value = sys_be32_to_cpu(reg_data);

    if (ret < 0) {
        LOG_ERR("spi_read FAIL %d\n", ret);
        return ret;
    }

    return 0;
}

/**
 * @brief Setup the pin direction (input or output)
 *
 * @param dev Device struct of the 165
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags) {
    if ((flags & GPIO_INPUT) == 0U) {
        return -ENOTSUP;
    }

    return 0;
}

static int reg_165_pin_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags) {
    int ret;

    /* Can't do SPI bus operations from an ISR */
    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    if ((flags & GPIO_OPEN_DRAIN) != 0U) {
        return -ENOTSUP;
    };

    ret = setup_pin_dir(dev, pin, flags);
    if (ret) {
        LOG_ERR("165: error setting pin direction (%d)", ret);
    }

    return ret;
}

static int reg_165_port_get_raw(const struct device *dev, uint32_t *value) {
    struct reg_165_drv_data *const drv_data = (struct reg_165_drv_data *const)dev->data;
    int ret;

    /* Can't do SPI bus operations from an ISR */
    if (k_is_in_isr()) {
        return -EWOULDBLOCK;
    }

    k_sem_take(&drv_data->lock, K_FOREVER);

    ret = reg_165_read_registers(dev, value);

    k_sem_give(&drv_data->lock);
    return ret;
}

static int reg_165_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value) {
    return -ENOTSUP;
}

static int reg_165_port_set_bits_raw(const struct device *dev, uint32_t mask) { return -ENOTSUP; }

static int reg_165_port_clear_bits_raw(const struct device *dev, uint32_t mask) { return -ENOTSUP; }

static int reg_165_port_toggle_bits(const struct device *dev, uint32_t mask) { return -ENOTSUP; }

static const struct gpio_driver_api api_table = {
    .pin_configure = reg_165_pin_config,
    .port_get_raw = reg_165_port_get_raw,
    .port_set_masked_raw = reg_165_port_set_masked_raw,
    .port_set_bits_raw = reg_165_port_set_bits_raw,
    .port_clear_bits_raw = reg_165_port_clear_bits_raw,
    .port_toggle_bits = reg_165_port_toggle_bits,
};

/**
 * @brief Initialization function of 165
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int reg_165_init(const struct device *dev) {
    const struct reg_165_config *const config = dev->config;
    struct reg_165_drv_data *const drv_data = (struct reg_165_drv_data *const)dev->data;

    if (!device_is_ready(config->bus.bus)) {
        LOG_ERR("Unable to get SPI bus device");
        return -ENODEV;
    }

    for (int i = 0; i < config->load_gpios_len; i++) {
        int err = gpio_pin_configure_dt(&config->load_gpios[i], GPIO_OUTPUT);
        if (err) {
            LOG_ERR("Unable to configure pin %u on %s for output", config->load_gpios[i].pin,
                    config->load_gpios[i].port->name);
            return err;
        }
        err = gpio_pin_set_dt(&config->load_gpios[i], 1);
        if (err) {
            LOG_ERR("Unable to set pin %u on %s to idle value '1'", config->load_gpios[i].pin,
                    config->load_gpios[i].port->name);
            return err;
        }
    }

    k_sem_init(&drv_data->lock, 1, 1);

    return 0;
}

#define GPIO_PORT_PIN_MASK_FROM_NGPIOS(ngpios) ((gpio_port_pins_t)(((uint64_t)1 << (ngpios)) - 1U))

#define GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)                                                      \
    GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_PROP(inst, ngpios))

#define REG_165_INIT(n)                                                                            \
    static const struct gpio_dt_spec load_gpios_##n[] = {                                          \
        COND_CODE_1(DT_INST_NODE_HAS_PROP(n, load_gpios),                                          \
                    (LISTIFY(LOAD_GPIOS_LEN(n), LOAD_GPIO_CFG_INIT, (, ), n)), ())};               \
    static struct reg_165_config reg_165_##n##_config = {                                          \
        .common =                                                                                  \
            {                                                                                      \
                .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),                               \
            },                                                                                     \
        .bus =                                                                                     \
            SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),   \
        .ngpios = DT_INST_PROP(n, ngpios),                                                         \
        .load_gpios = load_gpios_##n,                                                              \
        .load_gpios_len = ARRAY_SIZE(load_gpios_##n),                                              \
    };                                                                                             \
                                                                                                   \
    static struct reg_165_drv_data reg_165_##n##_drvdata = {};                                     \
                                                                                                   \
    /* This has to init after SPI master */                                                        \
    DEVICE_DT_INST_DEFINE(n, reg_165_init, NULL, &reg_165_##n##_drvdata, &reg_165_##n##_config,    \
                          POST_KERNEL, CONFIG_GPIO_165_INIT_PRIORITY, &api_table);

DT_INST_FOREACH_STATUS_OKAY(REG_165_INIT)
