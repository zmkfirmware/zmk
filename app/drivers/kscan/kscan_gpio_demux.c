/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_gpio_demux

#include <device.h>
#include <drivers/kscan.h>
#include <drivers/gpio.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct kscan_gpio_item_config {
    char *label;
    gpio_pin_t pin;
    gpio_flags_t flags;
};

// Helper macro
#define PWR_TWO(x) (1 << (x))

// Define GPIO cfg
#define _KSCAN_GPIO_ITEM_CFG_INIT(n, prop, idx)                                                    \
    {                                                                                              \
        .label = DT_INST_GPIO_LABEL_BY_IDX(n, prop, idx),                                          \
        .pin = DT_INST_GPIO_PIN_BY_IDX(n, prop, idx),                                              \
        .flags = DT_INST_GPIO_FLAGS_BY_IDX(n, prop, idx),                                          \
    },

// Define row and col cfg
#define _KSCAN_GPIO_INPUT_CFG_INIT(idx, n) _KSCAN_GPIO_ITEM_CFG_INIT(n, input_gpios, idx)
#define _KSCAN_GPIO_OUTPUT_CFG_INIT(idx, n) _KSCAN_GPIO_ITEM_CFG_INIT(n, output_gpios, idx)

// Check debounce config
#define CHECK_DEBOUNCE_CFG(n, a, b) COND_CODE_0(DT_INST_PROP(n, debounce_period), a, b)

// Define the row and column lengths
#define INST_MATRIX_INPUTS(n) DT_INST_PROP_LEN(n, input_gpios)
#define INST_DEMUX_GPIOS(n) DT_INST_PROP_LEN(n, output_gpios)
#define INST_MATRIX_OUTPUTS(n) PWR_TWO(INST_DEMUX_GPIOS(n))
#define POLL_INTERVAL(n) DT_INST_PROP(n, polling_interval_msec)

#define GPIO_INST_INIT(n)                                                                          \
    struct kscan_gpio_irq_callback_##n {                                                           \
        struct CHECK_DEBOUNCE_CFG(n, (k_work), (k_delayed_work)) * work;                           \
        struct gpio_callback callback;                                                             \
        const struct device *dev;                                                                  \
    };                                                                                             \
                                                                                                   \
    struct kscan_gpio_config_##n {                                                                 \
        struct kscan_gpio_item_config rows[INST_MATRIX_INPUTS(n)];                                 \
        struct kscan_gpio_item_config cols[INST_DEMUX_GPIOS(n)];                                   \
    };                                                                                             \
                                                                                                   \
    struct kscan_gpio_data_##n {                                                                   \
        kscan_callback_t callback;                                                                 \
        struct k_timer poll_timer;                                                                 \
        struct CHECK_DEBOUNCE_CFG(n, (k_work), (k_delayed_work)) work;                             \
        bool matrix_state[INST_MATRIX_INPUTS(n)][INST_MATRIX_OUTPUTS(n)];                          \
        const struct device *rows[INST_MATRIX_INPUTS(n)];                                          \
        const struct device *cols[INST_MATRIX_OUTPUTS(n)];                                         \
        const struct device *dev;                                                                  \
    };                                                                                             \
    /* IO/GPIO SETUP */                                                                            \
    /* gpio_input_devices are PHYSICAL IO devices */                                               \
    static const struct device **kscan_gpio_input_devices_##n(const struct device *dev) {          \
        struct kscan_gpio_data_##n *data = dev->data;                                              \
        return data->rows;                                                                         \
    }                                                                                              \
                                                                                                   \
    static const struct kscan_gpio_item_config *kscan_gpio_input_configs_##n(                      \
        const struct device *dev) {                                                                \
        const struct kscan_gpio_config_##n *cfg = dev->config;                                     \
        return cfg->rows;                                                                          \
    }                                                                                              \
                                                                                                   \
    /* gpio_output_devices are PHYSICAL IO devices */                                              \
    static const struct device **kscan_gpio_output_devices_##n(const struct device *dev) {         \
        struct kscan_gpio_data_##n *data = dev->data;                                              \
        return data->cols;                                                                         \
    }                                                                                              \
                                                                                                   \
    static const struct kscan_gpio_item_config *kscan_gpio_output_configs_##n(                     \
        const struct device *dev) {                                                                \
        const struct kscan_gpio_config_##n *cfg = dev->config;                                     \
        /* If row2col, rows = outputs & cols = inputs */                                           \
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
                const struct device *out_dev = kscan_gpio_output_devices_##n(dev)[bit];            \
                const struct kscan_gpio_item_config *out_cfg =                                     \
                    &kscan_gpio_output_configs_##n(dev)[bit];                                      \
                gpio_pin_set(out_dev, out_cfg->pin, state);                                        \
            }                                                                                      \
                                                                                                   \
            for (int i = 0; i < INST_MATRIX_INPUTS(n); i++) {                                      \
                /* Get the input device (port) */                                                  \
                const struct device *in_dev = kscan_gpio_input_devices_##n(dev)[i];                \
                /* Get the input device config (pin) */                                            \
                const struct kscan_gpio_item_config *in_cfg =                                      \
                    &kscan_gpio_input_configs_##n(dev)[i];                                         \
                read_state[i][o] = gpio_pin_get(in_dev, in_cfg->pin) > 0;                          \
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
            CHECK_DEBOUNCE_CFG(n, ({ k_work_submit(&data->work); }), ({                            \
                                   k_delayed_work_cancel(&data->work);                             \
                                   k_delayed_work_submit(&data->work, K_MSEC(5));                  \
                               }))                                                                 \
        }                                                                                          \
        return 0;                                                                                  \
    }                                                                                              \
                                                                                                   \
    static void kscan_gpio_work_handler_##n(struct k_work *work) {                                 \
        struct kscan_gpio_data_##n *data = CONTAINER_OF(work, struct kscan_gpio_data_##n, work);   \
        kscan_gpio_read_##n(data->dev);                                                            \
    }                                                                                              \
                                                                                                   \
    static struct kscan_gpio_data_##n kscan_gpio_data_##n = {                                      \
        .rows = {[INST_MATRIX_INPUTS(n) - 1] = NULL}, .cols = {[INST_DEMUX_GPIOS(n) - 1] = NULL}}; \
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
        const struct device **input_devices = kscan_gpio_input_devices_##n(dev);                   \
        for (int i = 0; i < INST_MATRIX_INPUTS(n); i++) {                                          \
            const struct kscan_gpio_item_config *in_cfg = &kscan_gpio_input_configs_##n(dev)[i];   \
            input_devices[i] = device_get_binding(in_cfg->label);                                  \
            if (!input_devices[i]) {                                                               \
                LOG_ERR("Unable to find input GPIO device");                                       \
                return -EINVAL;                                                                    \
            }                                                                                      \
            err = gpio_pin_configure(input_devices[i], in_cfg->pin, GPIO_INPUT | in_cfg->flags);   \
            if (err) {                                                                             \
                LOG_ERR("Unable to configure pin %d on %s for input", in_cfg->pin, in_cfg->label); \
                return err;                                                                        \
            } else {                                                                               \
                LOG_DBG("Configured pin %d on %s for input", in_cfg->pin, in_cfg->label);          \
            }                                                                                      \
            if (err) {                                                                             \
                LOG_ERR("Error adding the callback to the column device");                         \
                return err;                                                                        \
            }                                                                                      \
        }                                                                                          \
        /* configure output devices*/                                                              \
        const struct device **output_devices = kscan_gpio_output_devices_##n(dev);                 \
        for (int o = 0; o < INST_DEMUX_GPIOS(n); o++) {                                            \
            const struct kscan_gpio_item_config *out_cfg = &kscan_gpio_output_configs_##n(dev)[o]; \
            output_devices[o] = device_get_binding(out_cfg->label);                                \
            if (!output_devices[o]) {                                                              \
                LOG_ERR("Unable to find output GPIO device");                                      \
                return -EINVAL;                                                                    \
            }                                                                                      \
            err = gpio_pin_configure(output_devices[o], out_cfg->pin,                              \
                                     GPIO_OUTPUT_ACTIVE | out_cfg->flags);                         \
            if (err) {                                                                             \
                LOG_ERR("Unable to configure pin %d on %s for output", out_cfg->pin,               \
                        out_cfg->label);                                                           \
                return err;                                                                        \
            } else {                                                                               \
                LOG_DBG("Configured pin %d on %s for output", out_cfg->pin, out_cfg->label);       \
            }                                                                                      \
        }                                                                                          \
        data->dev = dev;                                                                           \
                                                                                                   \
        k_timer_init(&data->poll_timer, kscan_gpio_timer_handler, NULL);                           \
                                                                                                   \
        (CHECK_DEBOUNCE_CFG(n, (k_work_init), (k_delayed_work_init)))(                             \
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
        .rows = {UTIL_LISTIFY(INST_MATRIX_INPUTS(n), _KSCAN_GPIO_INPUT_CFG_INIT, n)},              \
        .cols = {UTIL_LISTIFY(INST_DEMUX_GPIOS(n), _KSCAN_GPIO_OUTPUT_CFG_INIT, n)},               \
    };                                                                                             \
                                                                                                   \
    DEVICE_AND_API_INIT(kscan_gpio_##n, DT_INST_LABEL(n), kscan_gpio_init_##n,                     \
                        &kscan_gpio_data_##n, &kscan_gpio_config_##n, APPLICATION,                 \
                        CONFIG_APPLICATION_INIT_PRIORITY, &gpio_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(GPIO_INST_INIT)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
