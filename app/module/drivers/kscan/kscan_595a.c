/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_595a

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define MAX_COLUMNS 64

struct kscan_595a_config {
    struct gpio_dt_spec ser_gpio;
    struct gpio_dt_spec sck_gpio;
    struct gpio_dt_spec sense_gpio;
    uint8_t hc595a_count;
};

struct kscan_595a_data {
    const struct device *dev;
    kscan_callback_t callback;
    struct k_work_delayable work;
    bool pressed[MAX_COLUMNS];
#if IS_ENABLED(CONFIG_PM_DEVICE)
    struct gpio_callback sense_cb;
#endif
};

static void kscan_595a_scan(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct kscan_595a_data *data = CONTAINER_OF(dwork, struct kscan_595a_data, work);
    const struct device *dev = data->dev;
    const struct kscan_595a_config *config = dev->config;

    const uint8_t num_columns = config->hc595a_count * 8;

    /*
     * Walking bit scan for 74HC595A shift registers.
     * With RCLK tied to VCC, outputs update immediately on clock edges.
     * DATA (SER) is kept HIGH normally. We walk a single LOW (0) through
     * the chain to select each column.
     */

    /* Initialize: Fill all outputs with 1s (inactive) */
    gpio_pin_set_dt(&config->ser_gpio, 1);
    for (int i = 0; i < num_columns; i++) {
        gpio_pin_set_dt(&config->sck_gpio, 1);
        gpio_pin_set_dt(&config->sck_gpio, 0);
    }

    /* Shift in a single 0 (active column) and walk it through */
    gpio_pin_set_dt(&config->ser_gpio, 0);
    gpio_pin_set_dt(&config->sck_gpio, 1);
    gpio_pin_set_dt(&config->sck_gpio, 0);
    gpio_pin_set_dt(&config->ser_gpio, 1);

    /* Scan each column */
    for (int col = 0; col < num_columns; col++) {
        /* Delay for signal to settle */
        k_busy_wait(10);

        /* Read sense pin - LOW means key pressed */
        bool pressed = (gpio_pin_get_dt(&config->sense_gpio) == 0);

        if (pressed != data->pressed[col]) {
            data->pressed[col] = pressed;
            if (data->callback) {
                data->callback(dev, 0, col, pressed);
            }
        }

        /* Shift to next column */
        gpio_pin_set_dt(&config->sck_gpio, 1);
        gpio_pin_set_dt(&config->sck_gpio, 0);
    }

    k_work_schedule(&data->work, K_MSEC(1));
}

static int kscan_595a_configure(const struct device *dev, kscan_callback_t callback) {
    struct kscan_595a_data *data = dev->data;
    data->callback = callback;
    return 0;
}

static int kscan_595a_enable(const struct device *dev) {
    struct kscan_595a_data *data = dev->data;
    k_work_schedule(&data->work, K_MSEC(100));
    return 0;
}

static int kscan_595a_disable(const struct device *dev) {
    struct kscan_595a_data *data = dev->data;
    k_work_cancel_delayable(&data->work);
    return 0;
}

static void kscan_595a_setup_pins(const struct device *dev) {
    const struct kscan_595a_config *config = dev->config;

    /* Configure SER GPIO (output, start HIGH) */
    gpio_pin_configure_dt(&config->ser_gpio, GPIO_OUTPUT_HIGH);

    /* Configure SCK GPIO (output, start LOW) */
    gpio_pin_configure_dt(&config->sck_gpio, GPIO_OUTPUT_LOW);

    /* Configure SENSE GPIO (input with pull-up) */
    gpio_pin_configure_dt(&config->sense_gpio, GPIO_INPUT | GPIO_PULL_UP);
}

#if IS_ENABLED(CONFIG_PM_DEVICE)

static void kscan_595a_sense_irq_handler(const struct device *gpio, struct gpio_callback *cb,
                                         uint32_t pins) {
    /* No-op: this callback exists to configure GPIO SENSE for nRF52 System OFF wake-up */
}

static void kscan_595a_set_all_outputs_low(const struct device *dev) {
    const struct kscan_595a_config *config = dev->config;
    const uint8_t num_columns = config->hc595a_count * 8;

    /*
     * Set all shift register outputs to LOW for sleep mode.
     * This way, any key press will pull the sense line LOW,
     * which can trigger a wake-up interrupt.
     */
    gpio_pin_set_dt(&config->ser_gpio, 0);
    for (int i = 0; i < num_columns; i++) {
        gpio_pin_set_dt(&config->sck_gpio, 1);
        gpio_pin_set_dt(&config->sck_gpio, 0);
    }
}

static int kscan_595a_sleep_gpios(const struct device *dev) {
    const struct kscan_595a_config *config = dev->config;
    int err;

    /* Keep SER and SCK pulled LOW during System OFF to prevent
     * noise-induced clock edges from corrupting shift register state.
     * nRF52 GPIO pull config is retained in the always-on domain. */
    err = gpio_pin_configure_dt(&config->ser_gpio, GPIO_INPUT | GPIO_PULL_DOWN);
    if (err) {
        return err;
    }

    err = gpio_pin_configure_dt(&config->sck_gpio, GPIO_INPUT | GPIO_PULL_DOWN);
    if (err) {
        return err;
    }

    return 0;
}

static int kscan_595a_pm_action(const struct device *dev, enum pm_device_action action) {
    struct kscan_595a_data *data = dev->data;
    const struct kscan_595a_config *config = dev->config;

    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        kscan_595a_disable(dev);
        /* Set all outputs LOW so any key press pulls sense LOW */
        kscan_595a_set_all_outputs_low(dev);
        /* Keep SER and SCK pulled LOW to preserve shift register state */
        kscan_595a_sleep_gpios(dev);
        /* Configure sense pin interrupt for wake-up from deep sleep.
         * On nRF52, this sets up GPIO SENSE for System OFF wake. */
        gpio_pin_interrupt_configure_dt(&config->sense_gpio, GPIO_INT_LEVEL_INACTIVE);
        gpio_init_callback(&data->sense_cb, kscan_595a_sense_irq_handler,
                           BIT(config->sense_gpio.pin));
        gpio_add_callback(config->sense_gpio.port, &data->sense_cb);
        return 0;

    case PM_DEVICE_ACTION_RESUME:
        gpio_pin_interrupt_configure_dt(&config->sense_gpio, GPIO_INT_DISABLE);
        gpio_remove_callback(config->sense_gpio.port, &data->sense_cb);
        kscan_595a_setup_pins(dev);
        return kscan_595a_enable(dev);

    default:
        return -ENOTSUP;
    }
}

#endif /* IS_ENABLED(CONFIG_PM_DEVICE) */

static int kscan_595a_init(const struct device *dev) {
    struct kscan_595a_data *data = dev->data;
    const struct kscan_595a_config *config = dev->config;

    data->dev = dev;
    memset(data->pressed, 0, sizeof(data->pressed));

    /* Verify GPIOs are ready */
    if (!gpio_is_ready_dt(&config->ser_gpio)) {
        LOG_ERR("SER GPIO not ready");
        return -ENODEV;
    }

    if (!gpio_is_ready_dt(&config->sck_gpio)) {
        LOG_ERR("SCK GPIO not ready");
        return -ENODEV;
    }

    if (!gpio_is_ready_dt(&config->sense_gpio)) {
        LOG_ERR("SENSE GPIO not ready");
        return -ENODEV;
    }

    k_work_init_delayable(&data->work, kscan_595a_scan);
    kscan_595a_setup_pins(dev);

    return 0;
}

static const struct kscan_driver_api kscan_595a_api = {
    .config = kscan_595a_configure,
    .enable_callback = kscan_595a_enable,
    .disable_callback = kscan_595a_disable,
};

#define KSCAN_595A_INIT(n)                                                                         \
    static struct kscan_595a_data kscan_595a_data_##n;                                             \
                                                                                                   \
    static const struct kscan_595a_config kscan_595a_config_##n = {                                \
        .ser_gpio = GPIO_DT_SPEC_INST_GET(n, hc595a_ser_gpios),                                    \
        .sck_gpio = GPIO_DT_SPEC_INST_GET(n, hc595a_sck_gpios),                                    \
        .sense_gpio = GPIO_DT_SPEC_INST_GET(n, key_sense_gpios),                                   \
        .hc595a_count = DT_INST_PROP(n, hc595a_count),                                             \
    };                                                                                             \
                                                                                                   \
    PM_DEVICE_DT_INST_DEFINE(n, kscan_595a_pm_action);                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, kscan_595a_init, PM_DEVICE_DT_INST_GET(n), &kscan_595a_data_##n,      \
                          &kscan_595a_config_##n, POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,         \
                          &kscan_595a_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_595A_INIT)
