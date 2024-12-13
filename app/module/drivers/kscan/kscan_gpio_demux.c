/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_gpio_demux

#include <zephyr/device.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Helper macro
#define PWR_TWO(x) (1 << (x))

// Define row and col cfg
#define _KSCAN_GPIO_CFG_INIT(n, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(n, prop, idx),

// Check debounce config
#define CHECK_DEBOUNCE_CFG(n, a, b) COND_CODE_0(DT_INST_PROP(n, debounce_period), a, b)

// Define the row and column lengths
#define INST_MATRIX_INPUTS(n) DT_INST_PROP_LEN(n, input_gpios)
#define INST_DEMUX_GPIOS(n) DT_INST_PROP_LEN(n, output_gpios)
#define INST_MATRIX_OUTPUTS(n) PWR_TWO(INST_DEMUX_GPIOS(n))
#define POLL_INTERVAL(n) DT_INST_PROP(n, polling_interval_msec)

#define GPIO_INST_INIT(n)                                                                          \
    struct kscan_gpio_irq_callback_##n {                                                           \
        struct CHECK_DEBOUNCE_CFG(n, (k_work), (k_work_delayable)) * work;                         \
        struct gpio_callback callback;                                                             \
        const struct device *dev;                                                                  \
    };                                                                                             \
                                                                                                   \
    struct kscan_gpio_config_##n {                                                                 \
        const struct gpio_dt_spec rows[INST_MATRIX_INPUTS(n)];                                     \
        const struct gpio_dt_spec cols[INST_DEMUX_GPIOS(n)];                                       \
    };                                                                                             \
                                                                                                   \
    struct kscan_gpio_data_##n {                                                                   \
        kscan_callback_t callback;                                                                 \
        struct k_timer poll_timer;                                                                 \
        struct CHECK_DEBOUNCE_CFG(n, (k_work), (k_work_delayable)) work;                           \
        bool matrix_state[INST_MATRIX_INPUTS(n)][INST_MATRIX_OUTPUTS(n)];                          \
        const struct device *dev;                                                                  \
    };                                                                                             \
    /* IO/GPIO SETUP */                                                                            \
    static const struct gpio_dt_spec *kscan_gpio_input_specs_##n(const struct device *dev) {       \
        const struct kscan_gpio_config_##n *cfg = dev->config;                                     \
        return cfg->rows;                                                                          \
    }                                                                                              \
                                                                                                   \
    static const struct gpio_dt_spec *kscan_gpio_output_specs_##n(const struct device *dev) {      \
        const struct kscan_gpio_config_##n *cfg = dev->config;                                     \
        return cfg->cols;                                                                          \
    }                                                                                              \
    /* POLLING SETUP */                                                                            \
    static void kscan_gpio_timer_handler(struct k_timer *timer) {                                  \
        struct kscan_gpio_data_##n *data =                                                         \
            CONTAINER_OF(timer, struct kscan_gpio_data_##n, poll_timer);                           \
        k_work_submit(&data->work.work);                                                           \
    }                                                                                              \
                                                                                                   \
    /* Read the state of the input GPIOs */                                                        \
    /* This is the core matrix_scan func */                                                        \
    static int kscan_gpio_read_##n(const struct device *dev) {                                     \
        bool submit_follow_up_read = false;                                                        \
        struct kscan_gpio_data_##n *data = dev->data;                                              \
        static bool read_state[INST_MATRIX_INPUTS(n)][INST_MATRIX_OUTPUTS(n)];                     \
        for (int o = 0; o < INST_MATRIX_OUTPUTS(n); o++) {                                         \
            /* Iterate over bits and set GPIOs accordingly */                                      \
            for (uint8_t bit = 0; bit < INST_DEMUX_GPIOS(n); bit++) {                              \
                uint8_t state = (o & (0b1 << bit)) >> bit;                                         \
                const struct gpio_dt_spec *out_spec = &kscan_gpio_output_specs_##n(dev)[bit];      \
                gpio_pin_set_dt(out_spec, state);                                                  \
            }                                                                                      \
            /* Let the col settle before reading the rows */                                       \
            k_usleep(1);                                                                           \
                                                                                                   \
            for (int i = 0; i < INST_MATRIX_INPUTS(n); i++) {                                      \
                /* Get the input spec */                                                           \
                const struct gpio_dt_spec *in_spec = &kscan_gpio_input_specs_##n(dev)[i];          \
                read_state[i][o] = gpio_pin_get_dt(in_spec) > 0;                                   \
            }                                                                                      \
        }                                                                                          \
        for (int r = 0; r < INST_MATRIX_INPUTS(n); r++) {                                          \
            for (int c = 0; c < INST_MATRIX_OUTPUTS(n); c++) {                                     \
                bool pressed = read_state[r][c];                                                   \
                submit_follow_up_read = (submit_follow_up_read || pressed);                        \
                if (pressed != data->matrix_state[r][c]) {                                         \
                    LOG_DBG("Sending event at %d,%d state %s", r, c, (pressed ? "on" : "off"));    \
                    data->matrix_state[r][c] = pressed;                                            \
                    data->callback(dev, r, c, pressed);                                            \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
        if (submit_follow_up_read) {                                                               \
            CHECK_DEBOUNCE_CFG(n, ({ k_work_submit(&data->work); }),                               \
                               ({ k_work_reschedule(&data->work, K_MSEC(5)); }))                   \
        }                                                                                          \
        return 0;                                                                                  \
    }                                                                                              \
                                                                                                   \
    static void kscan_gpio_work_handler_##n(struct k_work *work) {                                 \
        struct k_work_delayable *d_work = k_work_delayable_from_work(work);                        \
        struct kscan_gpio_data_##n *data = CONTAINER_OF(d_work, struct kscan_gpio_data_##n, work); \
        kscan_gpio_read_##n(data->dev);                                                            \
    }                                                                                              \
                                                                                                   \
    static struct kscan_gpio_data_##n kscan_gpio_data_##n = {};                                    \
                                                                                                   \
    /* KSCAN API configure function */                                                             \
    static int kscan_gpio_configure_##n(const struct device *dev, kscan_callback_t callback) {     \
        LOG_DBG("KSCAN API configure");                                                            \
        struct kscan_gpio_data_##n *data = dev->data;                                              \
        if (!callback) {                                                                           \
            return -EINVAL;                                                                        \
        }                                                                                          \
        data->callback = callback;                                                                 \
        LOG_DBG("Configured GPIO %d", n);                                                          \
        return 0;                                                                                  \
    };                                                                                             \
                                                                                                   \
    /* KSCAN API enable function */                                                                \
    static int kscan_gpio_enable_##n(const struct device *dev) {                                   \
        LOG_DBG("KSCAN API enable");                                                               \
        struct kscan_gpio_data_##n *data = dev->data;                                              \
        /* TODO: we might want a follow up to hook into the sleep state hooks in Zephyr, */        \
        /* and disable this timer when we enter a sleep state */                                   \
        k_timer_start(&data->poll_timer, K_MSEC(POLL_INTERVAL(n)), K_MSEC(POLL_INTERVAL(n)));      \
        return 0;                                                                                  \
    };                                                                                             \
                                                                                                   \
    /* KSCAN API disable function */                                                               \
    static int kscan_gpio_disable_##n(const struct device *dev) {                                  \
        LOG_DBG("KSCAN API disable");                                                              \
        struct kscan_gpio_data_##n *data = dev->data;                                              \
        k_timer_stop(&data->poll_timer);                                                           \
        return 0;                                                                                  \
    };                                                                                             \
                                                                                                   \
    /* GPIO init function*/                                                                        \
    static int kscan_gpio_init_##n(const struct device *dev) {                                     \
        LOG_DBG("KSCAN GPIO init");                                                                \
        struct kscan_gpio_data_##n *data = dev->data;                                              \
        int err;                                                                                   \
        /* configure input devices*/                                                               \
        for (int i = 0; i < INST_MATRIX_INPUTS(n); i++) {                                          \
            const struct gpio_dt_spec *in_spec = &kscan_gpio_input_specs_##n(dev)[i];              \
            if (!device_is_ready(in_spec->port)) {                                                 \
                LOG_ERR("Unable to find input GPIO device");                                       \
                return -EINVAL;                                                                    \
            }                                                                                      \
            err = gpio_pin_configure_dt(in_spec, GPIO_INPUT);                                      \
            if (err) {                                                                             \
                LOG_ERR("Unable to configure pin %d for input", in_spec->pin);                     \
                return err;                                                                        \
            } else {                                                                               \
                LOG_DBG("Configured pin %d for input", in_spec->pin);                              \
            }                                                                                      \
            if (err) {                                                                             \
                LOG_ERR("Error adding the callback to the column device");                         \
                return err;                                                                        \
            }                                                                                      \
        }                                                                                          \
        /* configure output devices*/                                                              \
        for (int o = 0; o < INST_DEMUX_GPIOS(n); o++) {                                            \
            const struct gpio_dt_spec *out_spec = &kscan_gpio_output_specs_##n(dev)[o];            \
            if (!device_is_ready(out_spec->port)) {                                                \
                LOG_ERR("Unable to find output GPIO device");                                      \
                return -EINVAL;                                                                    \
            }                                                                                      \
            err = gpio_pin_configure_dt(out_spec, GPIO_OUTPUT_ACTIVE);                             \
            if (err) {                                                                             \
                LOG_ERR("Unable to configure pin %d for output", out_spec->pin);                   \
                return err;                                                                        \
            } else {                                                                               \
                LOG_DBG("Configured pin %d for output", out_spec->pin);                            \
            }                                                                                      \
        }                                                                                          \
        data->dev = dev;                                                                           \
                                                                                                   \
        k_timer_init(&data->poll_timer, kscan_gpio_timer_handler, NULL);                           \
                                                                                                   \
        (CHECK_DEBOUNCE_CFG(n, (k_work_init), (k_work_init_delayable)))(                           \
            &data->work, kscan_gpio_work_handler_##n);                                             \
        return 0;                                                                                  \
    }                                                                                              \
                                                                                                   \
    static const struct kscan_driver_api gpio_driver_api_##n = {                                   \
        .config = kscan_gpio_configure_##n,                                                        \
        .enable_callback = kscan_gpio_enable_##n,                                                  \
        .disable_callback = kscan_gpio_disable_##n,                                                \
    };                                                                                             \
                                                                                                   \
    static const struct kscan_gpio_config_##n kscan_gpio_config_##n = {                            \
        .rows = {DT_FOREACH_PROP_ELEM(DT_DRV_INST(n), input_gpios, _KSCAN_GPIO_CFG_INIT)},         \
        .cols = {DT_FOREACH_PROP_ELEM(DT_DRV_INST(n), output_gpios, _KSCAN_GPIO_CFG_INIT)},        \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, kscan_gpio_init_##n, NULL, &kscan_gpio_data_##n,                      \
                          &kscan_gpio_config_##n, POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,         \
                          &gpio_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(GPIO_INST_INIT)
