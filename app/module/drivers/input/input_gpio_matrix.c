/*
 * Copyright (c) 2020-2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "input_gpio.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/pm/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include <zmk/debounce.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_input_gpio_matrix

#define INST_DIODE_DIR(n) DT_ENUM_IDX(DT_DRV_INST(n), diode_direction)
#define COND_DIODE_DIR(n, row2col_code, col2row_code)                                              \
    COND_CODE_0(INST_DIODE_DIR(n), row2col_code, col2row_code)

#define INST_ROWS_LEN(n) DT_INST_PROP_LEN(n, row_gpios)
#define INST_COLS_LEN(n) DT_INST_PROP_LEN(n, col_gpios)
#define INST_MATRIX_LEN(n) (INST_ROWS_LEN(n) * INST_COLS_LEN(n))
#define INST_INPUTS_LEN(n) COND_DIODE_DIR(n, (INST_COLS_LEN(n)), (INST_ROWS_LEN(n)))

#if CONFIG_ZMK_INPUT_DEBOUNCE_PRESS_MS >= 0
#define INST_DEBOUNCE_PRESS_MS(n) CONFIG_ZMK_INPUT_DEBOUNCE_PRESS_MS
#else
#define INST_DEBOUNCE_PRESS_MS(n)                                                                  \
    DT_INST_PROP_OR(n, debounce_period, DT_INST_PROP(n, debounce_press_ms))
#endif

#if CONFIG_ZMK_INPUT_DEBOUNCE_RELEASE_MS >= 0
#define INST_DEBOUNCE_RELEASE_MS(n) CONFIG_ZMK_INPUT_DEBOUNCE_RELEASE_MS
#else
#define INST_DEBOUNCE_RELEASE_MS(n)                                                                \
    DT_INST_PROP_OR(n, debounce_period, DT_INST_PROP(n, debounce_release_ms))
#endif

#define USE_POLLING IS_ENABLED(CONFIG_ZMK_INPUT_MATRIX_POLLING)
#define USE_INTERRUPTS (!USE_POLLING)

#define COND_INTERRUPTS(code) COND_CODE_1(CONFIG_ZMK_INPUT_MATRIX_POLLING, (), code)
#define COND_POLL_OR_INTERRUPTS(pollcode, intcode)                                                 \
    COND_CODE_1(CONFIG_ZMK_INPUT_MATRIX_POLLING, pollcode, intcode)

#define INPUT_GPIO_ROW_CFG_INIT(idx, inst_idx)                                                     \
    INPUT_GPIO_GET_BY_IDX(DT_DRV_INST(inst_idx), row_gpios, idx)
#define INPUT_GPIO_COL_CFG_INIT(idx, inst_idx)                                                     \
    INPUT_GPIO_GET_BY_IDX(DT_DRV_INST(inst_idx), col_gpios, idx)

enum input_diode_direction {
    INPUT_ROW2COL,
    INPUT_COL2ROW,
};

struct input_matrix_irq_callback {
    const struct device *dev;
    struct gpio_callback callback;
};

struct input_matrix_data {
    const struct device *dev;
    struct input_gpio_list inputs;
    struct k_work_delayable work;
#if USE_INTERRUPTS
    /** Array of length config->inputs.len */
    struct input_matrix_irq_callback *irqs;
#endif
    /** Timestamp of the current or scheduled scan. */
    int64_t scan_time;
    /**
     * Current state of the matrix as a flattened 2D array of length
     * (config->rows * config->cols)
     */
    struct zmk_debounce_state *matrix_state;
};

struct input_matrix_config {
    struct input_gpio_list outputs;
    struct zmk_debounce_config debounce_config;
    size_t rows;
    size_t cols;
    int32_t debounce_scan_period_ms;
    int32_t poll_period_ms;
    enum input_diode_direction diode_direction;
};

/**
 * Get the index into a matrix state array from a row and column.
 */
static int state_index_rc(const struct input_matrix_config *config, const int row, const int col) {
    __ASSERT(row < config->rows, "Invalid row %i", row);
    __ASSERT(col < config->cols, "Invalid column %i", col);

    return (col * config->rows) + row;
}

/**
 * Get the index into a matrix state array from input/output pin indices.
 */
static int state_index_io(const struct input_matrix_config *config, const int input_idx,
                          const int output_idx) {
    return (config->diode_direction == INPUT_ROW2COL)
               ? state_index_rc(config, output_idx, input_idx)
               : state_index_rc(config, input_idx, output_idx);
}

static int input_matrix_set_all_outputs(const struct device *dev, const int value) {
    const struct input_matrix_config *config = dev->config;

    for (int i = 0; i < config->outputs.len; i++) {
        const struct gpio_dt_spec *gpio = &config->outputs.gpios[i].spec;

        int err = gpio_pin_set_dt(gpio, value);
        if (err) {
            LOG_ERR("Failed to set output %i to %i: %i", i, value, err);
            return err;
        }
    }

    return 0;
}

#if USE_INTERRUPTS
static int input_matrix_interrupt_configure(const struct device *dev, const gpio_flags_t flags) {
    const struct input_matrix_data *data = dev->data;

    for (int i = 0; i < data->inputs.len; i++) {
        const struct gpio_dt_spec *gpio = &data->inputs.gpios[i].spec;

        int err = gpio_pin_interrupt_configure_dt(gpio, flags);
        if (err) {
            LOG_ERR("Unable to configure interrupt for pin %u on %s", gpio->pin, gpio->port->name);
            return err;
        }
    }

    return 0;
}
#endif

#if USE_INTERRUPTS
static int input_matrix_interrupt_enable(const struct device *dev) {
    int err = input_matrix_interrupt_configure(dev, GPIO_INT_LEVEL_ACTIVE);
    if (err) {
        return err;
    }

    // While interrupts are enabled, set all outputs active so a pressed key
    // will trigger an interrupt.
    return input_matrix_set_all_outputs(dev, 1);
}
#endif

#if USE_INTERRUPTS
static int input_matrix_interrupt_disable(const struct device *dev) {
    int err = input_matrix_interrupt_configure(dev, GPIO_INT_DISABLE);
    if (err) {
        return err;
    }

    // While interrupts are disabled, set all outputs inactive so
    // input_matrix_read() can scan them one by one.
    return input_matrix_set_all_outputs(dev, 0);
}
#endif

#if USE_INTERRUPTS
static void input_matrix_irq_callback_handler(const struct device *port, struct gpio_callback *cb,
                                              const gpio_port_pins_t pin) {
    struct input_matrix_irq_callback *irq_data =
        CONTAINER_OF(cb, struct input_matrix_irq_callback, callback);
    struct input_matrix_data *data = irq_data->dev->data;

    // Disable our interrupts temporarily to avoid re-entry while we scan.
    input_matrix_interrupt_disable(data->dev);

    data->scan_time = k_uptime_get();

    k_work_reschedule(&data->work, K_NO_WAIT);
}
#endif

static void input_matrix_read_continue(const struct device *dev) {
    const struct input_matrix_config *config = dev->config;
    struct input_matrix_data *data = dev->data;

    data->scan_time += config->debounce_scan_period_ms;

    k_work_reschedule(&data->work, K_TIMEOUT_ABS_MS(data->scan_time));
}

static void input_matrix_read_end(const struct device *dev) {
#if USE_INTERRUPTS
    // Return to waiting for an interrupt.
    input_matrix_interrupt_enable(dev);
#else
    struct input_matrix_data *data = dev->data;
    const struct input_matrix_config *config = dev->config;

    data->scan_time += config->poll_period_ms;

    // Return to polling slowly.
    k_work_reschedule(&data->work, K_TIMEOUT_ABS_MS(data->scan_time));
#endif
}

static int input_matrix_read(const struct device *dev) {
    struct input_matrix_data *data = dev->data;
    const struct input_matrix_config *config = dev->config;

    // Scan the matrix.
    for (int i = 0; i < config->outputs.len; i++) {
        const struct input_gpio *out_gpio = &config->outputs.gpios[i];

        int err = gpio_pin_set_dt(&out_gpio->spec, 1);
        if (err) {
            LOG_ERR("Failed to set output %i active: %i", out_gpio->index, err);
            return err;
        }

#if CONFIG_ZMK_INPUT_MATRIX_WAIT_BEFORE_INPUTS > 0
        k_busy_wait(CONFIG_ZMK_INPUT_MATRIX_WAIT_BEFORE_INPUTS);
#endif
        struct input_gpio_port_state state = {0};

        for (int j = 0; j < data->inputs.len; j++) {
            const struct input_gpio *in_gpio = &data->inputs.gpios[j];

            const int index = state_index_io(config, in_gpio->index, out_gpio->index);
            const int active = input_gpio_pin_get(in_gpio, &state);
            if (active < 0) {
                LOG_ERR("Failed to read port %s: %i", in_gpio->spec.port->name, active);
                return active;
            }

            zmk_debounce_update(&data->matrix_state[index], active, config->debounce_scan_period_ms,
                                &config->debounce_config);
        }

        err = gpio_pin_set_dt(&out_gpio->spec, 0);
        if (err) {
            LOG_ERR("Failed to set output %i inactive: %i", out_gpio->index, err);
            return err;
        }

#if CONFIG_ZMK_INPUT_MATRIX_WAIT_BETWEEN_OUTPUTS > 0
        k_busy_wait(CONFIG_ZMK_INPUT_MATRIX_WAIT_BETWEEN_OUTPUTS);
#endif
    }

    // Process the new state.
    bool continue_scan = false;

    for (int r = 0; r < config->rows; r++) {
        for (int c = 0; c < config->cols; c++) {
            const int index = state_index_rc(config, r, c);
            struct zmk_debounce_state *state = &data->matrix_state[index];

            if (zmk_debounce_get_changed(state)) {
                const bool pressed = zmk_debounce_is_pressed(state);

                LOG_DBG("Sending event at %i,%i state %s", r, c, pressed ? "on" : "off");
		input_report_abs(dev, INPUT_ABS_X, c, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Y, r, false, K_FOREVER);
		input_report_key(dev, INPUT_BTN_TOUCH, pressed, true, K_FOREVER);
            }

            continue_scan = continue_scan || zmk_debounce_is_active(state);
        }
    }

    if (continue_scan) {
        // At least one key is pressed or the debouncer has not yet decided if
        // it is pressed. Poll quickly until everything is released.
        input_matrix_read_continue(dev);
    } else {
        // All keys are released. Return to normal.
        input_matrix_read_end(dev);
    }

    return 0;
}

static void input_matrix_work_handler(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct input_matrix_data *data = CONTAINER_OF(dwork, struct input_matrix_data, work);
    input_matrix_read(data->dev);
}

static int input_matrix_enable(const struct device *dev) {
    struct input_matrix_data *data = dev->data;

    data->scan_time = k_uptime_get();

    // Read will automatically start interrupts/polling once done.
    return input_matrix_read(dev);
}

static int input_matrix_disable(const struct device *dev) {
    struct input_matrix_data *data = dev->data;

    k_work_cancel_delayable(&data->work);

#if USE_INTERRUPTS
    return input_matrix_interrupt_disable(dev);
#else
    return 0;
#endif
}

static int input_matrix_init_input_inst(const struct device *dev, const struct input_gpio *gpio) {
    if (!device_is_ready(gpio->spec.port)) {
        LOG_ERR("GPIO is not ready: %s", gpio->spec.port->name);
        return -ENODEV;
    }

    int err = gpio_pin_configure_dt(&gpio->spec, GPIO_INPUT);
    if (err) {
        LOG_ERR("Unable to configure pin %u on %s for input", gpio->spec.pin,
                gpio->spec.port->name);
        return err;
    }

    LOG_DBG("Configured pin %u on %s for input", gpio->spec.pin, gpio->spec.port->name);

#if USE_INTERRUPTS
    struct input_matrix_data *data = dev->data;
    struct input_matrix_irq_callback *irq = &data->irqs[gpio->index];

    irq->dev = dev;
    gpio_init_callback(&irq->callback, input_matrix_irq_callback_handler, BIT(gpio->spec.pin));
    err = gpio_add_callback(gpio->spec.port, &irq->callback);
    if (err) {
        LOG_ERR("Error adding the callback to the input device: %i", err);
        return err;
    }
#endif

    return 0;
}

static int input_matrix_init_inputs(const struct device *dev) {
    const struct input_matrix_data *data = dev->data;

    for (int i = 0; i < data->inputs.len; i++) {
        const struct input_gpio *gpio = &data->inputs.gpios[i];
        int err = input_matrix_init_input_inst(dev, gpio);
        if (err) {
            return err;
        }
    }

    return 0;
}

static int input_matrix_init_output_inst(const struct device *dev,
                                         const struct gpio_dt_spec *gpio) {
    if (!device_is_ready(gpio->port)) {
        LOG_ERR("GPIO is not ready: %s", gpio->port->name);
        return -ENODEV;
    }

    int err = gpio_pin_configure_dt(gpio, GPIO_OUTPUT);
    if (err) {
        LOG_ERR("Unable to configure pin %u on %s for output", gpio->pin, gpio->port->name);
        return err;
    }

    LOG_DBG("Configured pin %u on %s for output", gpio->pin, gpio->port->name);

    return 0;
}

static int input_matrix_init_outputs(const struct device *dev) {
    const struct input_matrix_config *config = dev->config;

    for (int i = 0; i < config->outputs.len; i++) {
        const struct gpio_dt_spec *gpio = &config->outputs.gpios[i].spec;
        int err = input_matrix_init_output_inst(dev, gpio);
        if (err) {
            return err;
        }
    }

    return 0;
}

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int input_matrix_disconnect_inputs(const struct device *dev) {
    const struct input_matrix_data *data = dev->data;

    for (int i = 0; i < data->inputs.len; i++) {
        const struct gpio_dt_spec *gpio = &data->inputs.gpios[i].spec;
        int err = gpio_pin_configure_dt(gpio, GPIO_DISCONNECTED);
        if (err) {
            return err;
        }
    }

    return 0;
}

static int input_matrix_disconnect_outputs(const struct device *dev) {
    const struct input_matrix_config *config = dev->config;

    for (int i = 0; i < config->outputs.len; i++) {
        const struct gpio_dt_spec *gpio = &config->outputs.gpios[i].spec;
        int err = gpio_pin_configure_dt(gpio, GPIO_DISCONNECTED);
        if (err) {
            return err;
        }
    }

    return 0;
}

#endif // IS_ENABLED(CONFIG_PM_DEVICE)

static void input_matrix_setup_pins(const struct device *dev) {
    input_matrix_init_inputs(dev);
    input_matrix_init_outputs(dev);
    input_matrix_set_all_outputs(dev, 0);
}

static int input_matrix_init(const struct device *dev) {
    struct input_matrix_data *data = dev->data;

    data->dev = dev;

    // Sort inputs by port so we can read each port just once per scan.
    input_gpio_list_sort_by_port(&data->inputs);

    k_work_init_delayable(&data->work, input_matrix_work_handler);

#if IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_init_suspended(dev);

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    pm_device_runtime_enable(dev);
#endif

#else
    input_matrix_setup_pins(dev);
#endif

    return 0;
}

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int input_matrix_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        input_matrix_disconnect_inputs(dev);
        input_matrix_disconnect_outputs(dev);

        return input_matrix_disable(dev);
    case PM_DEVICE_ACTION_RESUME:
        input_matrix_setup_pins(dev);
        return input_matrix_enable(dev);
    default:
        return -ENOTSUP;
    }
}

#endif // IS_ENABLED(CONFIG_PM_DEVICE)

#define INPUT_MATRIX_INIT(n)                                                                       \
    BUILD_ASSERT(INST_DEBOUNCE_PRESS_MS(n) <= DEBOUNCE_COUNTER_MAX,                                \
                 "ZMK_INPUT_DEBOUNCE_PRESS_MS or debounce-press-ms is too large");                 \
    BUILD_ASSERT(INST_DEBOUNCE_RELEASE_MS(n) <= DEBOUNCE_COUNTER_MAX,                              \
                 "ZMK_INPUT_DEBOUNCE_RELEASE_MS or debounce-release-ms is too large");             \
                                                                                                   \
    static struct input_gpio input_matrix_rows_##n[] = {                                           \
        LISTIFY(INST_ROWS_LEN(n), INPUT_GPIO_ROW_CFG_INIT, (, ), n)};                              \
                                                                                                   \
    static struct input_gpio input_matrix_cols_##n[] = {                                           \
        LISTIFY(INST_COLS_LEN(n), INPUT_GPIO_COL_CFG_INIT, (, ), n)};                              \
                                                                                                   \
    static struct zmk_debounce_state input_matrix_state_##n[INST_MATRIX_LEN(n)];                   \
                                                                                                   \
    COND_INTERRUPTS(                                                                               \
        (static struct input_matrix_irq_callback input_matrix_irqs_##n[INST_INPUTS_LEN(n)];))      \
                                                                                                   \
    static struct input_matrix_data input_matrix_data_##n = {                                      \
        .inputs =                                                                                  \
            INPUT_GPIO_LIST(COND_DIODE_DIR(n, (input_matrix_cols_##n), (input_matrix_rows_##n))),  \
        .matrix_state = input_matrix_state_##n,                                                    \
        COND_INTERRUPTS((.irqs = input_matrix_irqs_##n, ))};                                       \
                                                                                                   \
    static const struct input_matrix_config input_matrix_config_##n = {                            \
        .rows = ARRAY_SIZE(input_matrix_rows_##n),                                                 \
        .cols = ARRAY_SIZE(input_matrix_cols_##n),                                                 \
        .outputs =                                                                                 \
            INPUT_GPIO_LIST(COND_DIODE_DIR(n, (input_matrix_rows_##n), (input_matrix_cols_##n))),  \
        .debounce_config =                                                                         \
            {                                                                                      \
                .debounce_press_ms = INST_DEBOUNCE_PRESS_MS(n),                                    \
                .debounce_release_ms = INST_DEBOUNCE_RELEASE_MS(n),                                \
            },                                                                                     \
        .debounce_scan_period_ms = DT_INST_PROP(n, debounce_scan_period_ms),                       \
        .poll_period_ms = DT_INST_PROP(n, poll_period_ms),                                         \
        .diode_direction = INST_DIODE_DIR(n),                                                      \
    };                                                                                             \
                                                                                                   \
    PM_DEVICE_DT_INST_DEFINE(n, input_matrix_pm_action);                                           \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, &input_matrix_init, PM_DEVICE_DT_INST_GET(n), &input_matrix_data_##n, \
                          &input_matrix_config_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_MATRIX_INIT);
