/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_ext_power_generic

#include <device.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/ext_power.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct ext_power_generic_config {
    const char *label;
    const u8_t pin;
    const u8_t flags;
};

struct ext_power_generic_data {
    struct device *gpio;
    bool status;
};

static int ext_power_generic_enable(struct device *dev) {
    struct ext_power_generic_data *data = dev->driver_data;
    const struct ext_power_generic_config *config = dev->config_info;

    if (gpio_pin_set(data->gpio, config->pin, 1)) {
        LOG_WRN("Failed to set ext-power control pin");
        return -EIO;
    }
    data->status = true;
    return 0;
}

static int ext_power_generic_disable(struct device *dev) {
    struct ext_power_generic_data *data = dev->driver_data;
    const struct ext_power_generic_config *config = dev->config_info;

    if (gpio_pin_set(data->gpio, config->pin, 0)) {
        LOG_WRN("Failed to clear ext-power control pin");
        return -EIO;
    }
    data->status = false;
    return 0;
}

static int ext_power_generic_get(struct device *dev) {
    struct ext_power_generic_data *data = dev->driver_data;
    return data->status;
}

static int ext_power_generic_init(struct device *dev) {
    struct ext_power_generic_data *data = dev->driver_data;
    const struct ext_power_generic_config *config = dev->config_info;

    data->gpio = device_get_binding(config->label);
    if (data->gpio == NULL) {
        LOG_ERR("Failed to get ext-power control device");
        return -EINVAL;
    }

    if (gpio_pin_configure(data->gpio, config->pin, config->flags | GPIO_OUTPUT)) {
        LOG_ERR("Failed to configure ext-power control pin");
        return -EIO;
    }

    return 0;
}

static const struct ext_power_generic_config config = {
    .label = DT_INST_GPIO_LABEL(0, control_gpios),
    .pin = DT_INST_GPIO_PIN(0, control_gpios),
    .flags = DT_INST_GPIO_FLAGS(0, control_gpios)};

static struct ext_power_generic_data data = {.status = false};

static const struct ext_power_api api = {.enable = ext_power_generic_enable,
                                         .disable = ext_power_generic_disable,
                                         .get = ext_power_generic_get};

DEVICE_AND_API_INIT(ext_power_generic, DT_INST_LABEL(0), ext_power_generic_init, &data, &config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
