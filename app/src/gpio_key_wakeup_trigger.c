/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_gpio_key_wakeup_trigger

struct gpio_key_wakeup_trigger_config {
    struct gpio_dt_spec trigger;
    size_t extra_gpios_count;
    struct gpio_dt_spec extra_gpios[];
};

static int zmk_gpio_key_wakeup_trigger_init(const struct device *dev) {
#if IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_init_suspended(dev);
    pm_device_wakeup_enable(dev, true);
#endif

    return 0;
}

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int gpio_key_wakeup_trigger_pm_resume(const struct device *dev) {
    const struct gpio_key_wakeup_trigger_config *config = dev->config;

    int ret = gpio_pin_configure_dt(&config->trigger, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure wakeup trigger key GPIO pin as input (%d)", ret);
        return ret;
    }
    ret = gpio_pin_interrupt_configure_dt(&config->trigger, GPIO_INT_LEVEL_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure wakeup trigger key GPIO pin interrupt (%d)", ret);
        return ret;
    }

    for (int i = 0; i < config->extra_gpios_count; i++) {
        ret = gpio_pin_configure_dt(&config->extra_gpios[i], GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_WRN("Failed to set extra GPIO pin active for waker (%d)", ret);
            return ret;
        }
    }

    return ret;
}

static int gpio_key_wakeup_trigger_pm_suspend(const struct device *dev) {
    const struct gpio_key_wakeup_trigger_config *config = dev->config;

    int ret = gpio_pin_interrupt_configure_dt(&config->trigger, GPIO_INT_DISABLE);
    if (ret < 0) {
        LOG_ERR("Failed to configure wakeup trigger key GPIO pin interrupt (%d)", ret);
    }

    for (int i = 0; i < config->extra_gpios_count; i++) {
        ret = gpio_pin_configure_dt(&config->extra_gpios[i], GPIO_DISCONNECTED);
        if (ret < 0) {
            LOG_WRN("Failed to set extra GPIO pin disconnected for waker (%d)", ret);
            return ret;
        }
    }

    return ret;
}

// The waker is "backwards", in as much as it is designed to be resumed/enabled immediately
// before a soft-off state is entered, so it can wake the device from that state later.
// So this waker correctly resumes and is ready to wake the device later.
static int gpio_key_wakeup_trigger_pm_action(const struct device *dev,
                                             enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_RESUME:
        return gpio_key_wakeup_trigger_pm_resume(dev);
    case PM_DEVICE_ACTION_SUSPEND:
        return gpio_key_wakeup_trigger_pm_suspend(dev);
    default:
        return -ENOTSUP;
    }
}

#endif // IS_ENABLED(CONFIG_PM_DEVICE)

#define WAKEUP_TRIGGER_EXTRA_GPIO_SPEC(idx, n)                                                     \
    GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), extra_gpios, idx)

#define GPIO_KEY_WAKEUP_TRIGGER_INST(n)                                                            \
    const struct gpio_key_wakeup_trigger_config wtk_cfg_##n = {                                    \
        .trigger = GPIO_DT_SPEC_GET(DT_INST_PROP(n, trigger), gpios),                              \
        .extra_gpios = {LISTIFY(DT_PROP_LEN_OR(DT_DRV_INST(n), extra_gpios, 0),                    \
                                WAKEUP_TRIGGER_EXTRA_GPIO_SPEC, (, ), n)},                         \
        .extra_gpios_count = DT_PROP_LEN_OR(DT_DRV_INST(n), extra_gpios, 0),                       \
    };                                                                                             \
    PM_DEVICE_DT_INST_DEFINE(n, gpio_key_wakeup_trigger_pm_action);                                \
    DEVICE_DT_INST_DEFINE(n, zmk_gpio_key_wakeup_trigger_init, PM_DEVICE_DT_INST_GET(n), NULL,     \
                          &wtk_cfg_##n, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_KEY_WAKEUP_TRIGGER_INST)
