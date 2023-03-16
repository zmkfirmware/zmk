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

#define DT_DRV_COMPAT zmk_soft_on_off_gpio

struct soft_on_off_config {
    struct gpio_dt_spec input_gpio;
    struct gpio_callback callback;
};

#define WAKEUP_SOURCE_AND_COMMA(node, prop, idx) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node, prop, idx)),

#define WAKEUP_SOURCES_NUM DT_INST_PROP_LEN(0, wakeup_sources)
const struct device *wakeup_sources[WAKEUP_SOURCES_NUM] = {
    DT_INST_FOREACH_PROP_ELEM(0, wakeup_sources, WAKEUP_SOURCE_AND_COMMA)};

#define OUTPUT_AND_COMMA(node, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(node, prop, idx),

#if DT_INST_NODE_HAS_PROP(0, output_gpios)
const struct gpio_dt_spec outputs[DT_INST_PROP_LEN(0, output_gpios)] = {
    DT_INST_FOREACH_PROP_ELEM(0, output_gpios, OUTPUT_AND_COMMA)};

#endif // DT_INST_NODE_HAS_PROP(0, output_gpios)

static struct soft_on_off_config config = {
    .input_gpio = GPIO_DT_SPEC_INST_GET(0, input_gpios),
};

static void zmk_soft_on_off_pressed_work_cb(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(zmk_soft_on_off_gpio_work, zmk_soft_on_off_pressed_work_cb);

static void zmk_soft_on_off_pressed_work_cb(struct k_work *work) {
    int err;

    // Delay again if our pin is still active
    if (gpio_pin_get_dt(&config.input_gpio) > 0) {
        LOG_DBG("soft-on-off work cbt: pin still enabled");
        k_work_schedule(&zmk_soft_on_off_gpio_work, K_SECONDS(1));
    }

#if IS_ENABLED(CONFIG_PM_DEVICE)
    // There may be some matrix/direct kscan devices that would be used for wakeup
    // from normal "inactive goes to sleep" behavior, so disable them as wakeup devices
    // and then suspend them so we're ready to take over setting up our system
    // and then putting it into an off state.
    for (int i = 0; i < WAKEUP_SOURCES_NUM; i++) {
        const struct device *dev = wakeup_sources[i];

        LOG_DBG("soft-on-off pressed cb: suspend device");
        if (pm_device_wakeup_is_capable(dev)) {
            pm_device_wakeup_enable(dev, false);
        }
        pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
    }

#endif // IS_ENABLED(CONFIG_PM_DEVICE)

#if DT_INST_NODE_HAS_PROP(0, output_gpios)
    for (int i = 0; i < ARRAY_SIZE(outputs); i++) {
        const struct gpio_dt_spec *spec = &outputs[i];

        LOG_DBG("soft-on-off pressed cb: setting output active");
        gpio_pin_configure_dt(spec, GPIO_OUTPUT_ACTIVE);
    }
#endif // DT_INST_NODE_HAS_PROP(0, output_gpios)

    err = gpio_remove_callback(config.input_gpio.port, &config.callback);
    if (err) {
        LOG_ERR("Error removing the callback to the soft on_off GPIO input device: %i", err);
        return;
    }

    err = gpio_pin_interrupt_configure_dt(&config.input_gpio, GPIO_INT_LEVEL_ACTIVE);
    if (err < 0) {
        LOG_ERR("Failed to configure soft on_off GPIO pin interrupt (%d)", err);
        return;
    }

    LOG_DBG("soft-on-off interrupt: go to sleep");
    pm_state_force(0U, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
}

static void zmk_soft_on_off_gpio_interrupt_cb(const struct device *port, struct gpio_callback *cb,
                                              const gpio_port_pins_t pin) {
    // Some super simple debounce, since our interrupt may be triggered by scanning of some matrix,
    // and we can't schedule our reads to debounce perfectly with any active scanning going on
    // in parallel.
    LOG_DBG("soft-on-off interrupt: %d vs %d", pin, config.input_gpio.pin);
    if ((pin & (gpio_port_pins_t)BIT(config.input_gpio.pin)) != 0 &&
        gpio_pin_get_dt(&config.input_gpio) > 0) {
        LOG_DBG("soft-on-off scheduling the work");
        int err = gpio_pin_interrupt_configure_dt(&config.input_gpio, GPIO_INT_DISABLE);
        if (err < 0) {
            LOG_ERR("Failed to disable soft on_off GPIO pin interrupt (%d)", err);
            return;
        }
        k_work_schedule(&zmk_soft_on_off_gpio_work, K_SECONDS(2));
    }
}

static void zmk_soft_on_off_finish_init(struct k_work *work) {
    int err;

    gpio_init_callback(&config.callback, zmk_soft_on_off_gpio_interrupt_cb,
                       BIT(config.input_gpio.pin));
    err = gpio_add_callback(config.input_gpio.port, &config.callback);
    if (err) {
        LOG_ERR("Error adding the callback to the soft on_off GPIO input device: %i", err);
        return;
    }

    err = gpio_pin_interrupt_configure_dt(&config.input_gpio, GPIO_INT_LEVEL_ACTIVE);
    if (err < 0) {
        LOG_ERR("Failed to configure soft on_off GPIO pin interrupt (%d)", err);
        return;
    }
}

K_WORK_DELAYABLE_DEFINE(finish_init_work, zmk_soft_on_off_finish_init);

static int zmk_soft_on_off_gpio_init(const struct device *_arg) {
    int err;

    err = gpio_pin_configure_dt(&config.input_gpio, GPIO_INPUT);
    if (err < 0) {
        LOG_ERR("Failed to configure soft on_off GPIO pin for input (%d)", err);
        return err;
    }

    k_work_schedule(&finish_init_work, K_SECONDS(2));

    return 0;
}

SYS_INIT(zmk_soft_on_off_gpio_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
