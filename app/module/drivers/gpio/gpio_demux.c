/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_gpio_demux

/**
 * @file Driver for demux GPIO driver.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_demux);

#define GPIO_DEMUX_NO_PIN UINT8_MAX

/** Configuration data */
struct gpio_demux_config {
    /* gpio_driver_data needs to be first */
    struct gpio_driver_config common;

    struct gpio_dt_spec en_gpio;
    uint8_t sel_gpios_len;
    struct gpio_dt_spec sel_gpios[];
};

/** Runtime driver data */
struct gpio_demux_data {
    /* gpio_driver_data needs to be first */
    struct gpio_driver_config data;

    gpio_pin_t active_pin;
};

/**
 * @brief Setup the pin direction (input or output)
 *
 * @param dev Device struct of the demux
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags) {
    const struct gpio_demux_config *const cfg = dev->config;
    struct gpio_demux_data *const data = dev->data;
    int ret;

    if (flags & GPIO_INPUT) {
        return -ENOTSUP;
    } else if (!(flags & GPIO_OUTPUT) && pin == data->active_pin) {
        if (cfg->en_gpio.port) {
            ret = gpio_pin_set_dt(&cfg->en_gpio, 0);
            if (ret < 0) {
                LOG_ERR("Failed to disable the en-gpio");
                return ret;
            }
        }
        data->active_pin = GPIO_DEMUX_NO_PIN;
    }

    return 0;
}

static int gpio_demux_port_get_raw(const struct device *dev, uint32_t *value) { return -ENOTSUP; }

static int gpio_demux_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value) {
    const struct gpio_demux_config *const cfg = dev->config;
    struct gpio_demux_data *const data = dev->data;
    int ret;
    int pin = 0;

    if (cfg->en_gpio.port) {
        ret = gpio_pin_set_dt(&cfg->en_gpio, 0);
	if (ret) {
            return ret;
	}
    }

    if (!(mask & value)) {
	    LOG_DBG("Clearing the active pin!");
        data->active_pin = GPIO_DEMUX_NO_PIN;
        return 0;
    }

    while (!IS_BIT_SET(mask, 0)) {
	    pin++;
	    mask = mask >> 1;
    }

    if (data->active_pin != GPIO_DEMUX_NO_PIN && data->active_pin != pin) {
	    LOG_ERR("demux can only set one pin active at a time");
	    return -ENOMEM;
    }

    data->active_pin = pin;
    for (int i = 0; i < cfg->sel_gpios_len; i++) {
        int val = (pin & BIT(i)) != 0 ? 1 : 0;
        gpio_pin_set_dt(&cfg->sel_gpios[i], val);
    }

    if (cfg->en_gpio.port) {
        ret = gpio_pin_set_dt(&cfg->en_gpio, 1);
    }

    return ret;
}

static int gpio_demux_port_set_bits_raw(const struct device *dev, uint32_t mask) {
    return gpio_demux_port_set_masked_raw(dev, mask, mask);
}

static int gpio_demux_port_clear_bits_raw(const struct device *dev, uint32_t mask) {
    return gpio_demux_port_set_masked_raw(dev, mask, 0);
}

static int gpio_demux_pin_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags) {
    int ret;

    if ((flags & GPIO_OPEN_DRAIN) != 0U) {
        return -ENOTSUP;
    };

    ret = setup_pin_dir(dev, pin, flags);
    if (ret) {
        LOG_ERR("demux: error setting pin direction (%d)", ret);
    }

    if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
        return gpio_demux_port_clear_bits_raw(dev, BIT(pin));
    } else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
        return gpio_demux_port_set_bits_raw(dev, BIT(pin));
    }

    return ret;
}

static const struct gpio_driver_api api_table = {
    .pin_configure = gpio_demux_pin_config,
    .port_get_raw = gpio_demux_port_get_raw,
    .port_set_masked_raw = gpio_demux_port_set_masked_raw,
    .port_set_bits_raw = gpio_demux_port_set_bits_raw,
    .port_clear_bits_raw = gpio_demux_port_clear_bits_raw,
};

/**
 * @brief Initialization function of demux
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_demux_init(const struct device *dev) {
    const struct gpio_demux_config *const cfg = dev->config;
    struct gpio_demux_data *const data = dev->data;
    int ret;

    for (size_t i = 0; i < cfg->sel_gpios_len; i++) {
        ret = gpio_pin_configure_dt(&cfg->sel_gpios[i], GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("Failed to configure select pin as an output");
		return ret;
	}
    }

    if (cfg->en_gpio.port) {
        ret = gpio_pin_configure_dt(&cfg->en_gpio, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("Failed to configure enable pin as an output");
		return ret;
	}
    }

    data->active_pin = GPIO_DEMUX_NO_PIN;

    return 0;
}

#define GPIO_PORT_PIN_MASK_FROM_NGPIOS(ngpios) ((gpio_port_pins_t)(((uint64_t)1 << (ngpios)) - 1U))

#define GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)                                                      \
    GPIO_PORT_PIN_MASK_FROM_NGPIOS(1 << DT_INST_PROP_LEN(inst, select_gpios))

#define GPIO_DEMUX_INIT(n)                                                                            \
    static const struct gpio_demux_config gpio_demux_##n##_config = {                                    \
        .common =                                                                                  \
            {                                                                                      \
                .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),                               \
            },                                                                                     \
        .en_gpio = GPIO_DT_SPEC_INST_GET_OR(n, en_gpios, {}), \
        .sel_gpios = {DT_FOREACH_PROP_ELEM_SEP(DT_DRV_INST(n), select_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))}, \
        .sel_gpios_len = DT_INST_PROP_LEN(n, select_gpios), \
    };                                                                                             \
                                                                                                   \
    static struct gpio_demux_data gpio_demux_##n##_drvdata = {};                                     \
                                                                                                   \
    /* This has to init after SPI master */                                                        \
    DEVICE_DT_INST_DEFINE(n, gpio_demux_init, NULL, &gpio_demux_##n##_drvdata, &gpio_demux_##n##_config,    \
                          POST_KERNEL, CONFIG_GPIO_DEMUX_INIT_PRIORITY, &api_table);

DT_INST_FOREACH_STATUS_OKAY(GPIO_DEMUX_INIT)
