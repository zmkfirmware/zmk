/*
 * Copyright (c) 2020-2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/kscan.h>
#include <logging/log.h>
#include <sys/__assert.h>
#include <sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_kscan_gpio_matrix

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define INST_DIODE_DIR(n) DT_ENUM_IDX(DT_DRV_INST(n), diode_direction)
#define COND_DIODE_DIR(n, row2col_code, col2row_code)                                              \
    COND_CODE_0(INST_DIODE_DIR(n), row2col_code, col2row_code)

#define INST_ROWS_LEN(n) DT_INST_PROP_LEN(n, row_gpios)
#define INST_COLS_LEN(n) DT_INST_PROP_LEN(n, col_gpios)
#define INST_MATRIX_LEN(n) (INST_ROWS_LEN(n) * INST_COLS_LEN(n))
#define INST_INPUTS_LEN(n) COND_DIODE_DIR(n, (INST_COLS_LEN(n)), (INST_ROWS_LEN(n)))

#define USE_POLLING IS_ENABLED(CONFIG_ZMK_KSCAN_MATRIX_POLLING)
#define USE_INTERRUPTS (!USE_POLLING)

#define COND_INTERRUPTS(code) COND_CODE_1(CONFIG_ZMK_KSCAN_MATRIX_POLLING, (), code)
#define COND_POLL_OR_INTERRUPTS(pollcode, intcode)                                                 \
    COND_CODE_1(CONFIG_ZMK_KSCAN_MATRIX_POLLING, pollcode, intcode)

// TODO (Zephr 2.6): replace the following
// kscan_gpio_dt_spec -> gpio_dt_spec
// KSCAN_GPIO_DT_SPEC_GET_BY_IDX -> GPIO_DT_SPEC_GET_BY_IDX
// gpio_pin_get -> gpio_pin_get_dt
// gpio_pin_set -> gpio_pin_set_dt
// gpio_pin_interrupt_configure -> gpio_pin_interrupt_configure_dt
struct kscan_gpio_dt_spec {
    const struct device *port;
    gpio_pin_t pin;
    gpio_dt_flags_t dt_flags;
};

#define KSCAN_GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)                                          \
    {                                                                                              \
        .port = DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(node_id, prop, idx)),                            \
        .pin = DT_GPIO_PIN_BY_IDX(node_id, prop, idx),                                             \
        .dt_flags = DT_GPIO_FLAGS_BY_IDX(node_id, prop, idx),                                      \
    }

#define KSCAN_GPIO_ROW_CFG_INIT(idx, inst_idx)                                                     \
    KSCAN_GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst_idx), row_gpios, idx),
#define KSCAN_GPIO_COL_CFG_INIT(idx, inst_idx)                                                     \
    KSCAN_GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst_idx), col_gpios, idx),

enum kscan_diode_direction {
    KSCAN_ROW2COL,
    KSCAN_COL2ROW,
};

struct kscan_matrix_irq_callback {
    const struct device *dev;
    struct gpio_callback callback;
    struct k_delayed_work *work;
};

struct kscan_matrix_data {
    const struct device *dev;
    kscan_callback_t callback;
    struct k_delayed_work work;
#if USE_POLLING
    struct k_timer poll_timer;
#else
    /** Array of length config->inputs.len */
    struct kscan_matrix_irq_callback *irqs;
#endif
    /**
     * Current state of the matrix as a flattened 2D array of length
     * (config->rows.len * config->cols.len)
     */
    bool *current_state;
    /** Buffer for reading in the next matrix state. Parallel array to current_state. */
    bool *next_state;
};

struct kscan_gpio_list {
    const struct kscan_gpio_dt_spec *gpios;
    size_t len;
};

/** Define a kscan_gpio_list from a compile-time GPIO array. */
#define KSCAN_GPIO_LIST(gpio_array)                                                                \
    ((struct kscan_gpio_list){.gpios = gpio_array, .len = ARRAY_SIZE(gpio_array)})

struct kscan_matrix_config {
    struct kscan_gpio_list rows;
    struct kscan_gpio_list cols;
    struct kscan_gpio_list inputs;
    struct kscan_gpio_list outputs;
    int32_t debounce_period_ms;
    int32_t poll_period_ms;
    enum kscan_diode_direction diode_direction;
};

/**
 * Get the index into a matrix state array from a row and column.
 */
static int state_index_rc(const struct kscan_matrix_config *config, const int row, const int col) {
    __ASSERT(row < config->rows.len, "Invalid row %i", row);
    __ASSERT(col < config->cols.len, "Invalid column %i", col);

    return (col * config->rows.len) + row;
}

/**
 * Get the index into a matrix state array from input/output pin indices.
 */
static int state_index_io(const struct kscan_matrix_config *config, const int input_idx,
                          const int output_idx) {
    return (config->diode_direction == KSCAN_ROW2COL)
               ? state_index_rc(config, output_idx, input_idx)
               : state_index_rc(config, input_idx, output_idx);
}

static int kscan_matrix_set_all_outputs(const struct device *dev, const int value) {
    const struct kscan_matrix_config *config = dev->config;

    for (int i = 0; i < config->outputs.len; i++) {
        const struct kscan_gpio_dt_spec *gpio = &config->outputs.gpios[i];

        int err = gpio_pin_set(gpio->port, gpio->pin, value);
        if (err) {
            LOG_ERR("Failed to set output %i to %i: %i", i, value, err);
            return err;
        }
    }

    return 0;
}

#if USE_INTERRUPTS
static int kscan_matrix_interrupt_configure(const struct device *dev, const gpio_flags_t flags) {
    const struct kscan_matrix_config *config = dev->config;

    for (int i = 0; i < config->inputs.len; i++) {
        const struct kscan_gpio_dt_spec *gpio = &config->inputs.gpios[i];

        int err = gpio_pin_interrupt_configure(gpio->port, gpio->pin, flags);
        if (err) {
            LOG_ERR("Unable to configure interrupt for pin %u on %s", gpio->pin, gpio->port->name);
            return err;
        }
    }

    return 0;
}
#endif

#if USE_INTERRUPTS
static int kscan_matrix_interrupt_enable(const struct device *dev) {
    int err = kscan_matrix_interrupt_configure(dev, GPIO_INT_LEVEL_ACTIVE);
    if (err) {
        return err;
    }

    // While interrupts are enabled, set all outputs active so a pressed key
    // will trigger an interrupt.
    return kscan_matrix_set_all_outputs(dev, 1);
}
#endif

#if USE_INTERRUPTS
static int kscan_matrix_interrupt_disable(const struct device *dev) {
    int err = kscan_matrix_interrupt_configure(dev, GPIO_INT_DISABLE);
    if (err) {
        return err;
    }

    // While interrupts are disabled, set all outputs inactive so
    // kscan_matrix_read() can scan them one by one.
    return kscan_matrix_set_all_outputs(dev, 0);
}
#endif

#if USE_INTERRUPTS
static void kscan_matrix_irq_callback_handler(const struct device *port, struct gpio_callback *cb,
                                              const gpio_port_pins_t pin) {
    struct kscan_matrix_irq_callback *data =
        CONTAINER_OF(cb, struct kscan_matrix_irq_callback, callback);
    const struct kscan_matrix_config *config = data->dev->config;

    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_matrix_interrupt_disable(data->dev);

    // TODO (Zephyr 2.6): use k_work_reschedule()
    k_delayed_work_cancel(data->work);
    k_delayed_work_submit(data->work, K_MSEC(config->debounce_period_ms));
}
#endif

static int kscan_matrix_read(const struct device *dev) {
    struct kscan_matrix_data *data = dev->data;
    const struct kscan_matrix_config *config = dev->config;

    // Scan the matrix.
    for (int o = 0; o < config->outputs.len; o++) {
        const struct kscan_gpio_dt_spec *out_gpio = &config->outputs.gpios[o];

        int err = gpio_pin_set(out_gpio->port, out_gpio->pin, 1);
        if (err) {
            LOG_ERR("Failed to set output %i active: %i", o, err);
            return err;
        }

        for (int i = 0; i < config->inputs.len; i++) {
            const struct kscan_gpio_dt_spec *in_gpio = &config->inputs.gpios[i];

            const int index = state_index_io(config, i, o);
            data->next_state[index] = gpio_pin_get(in_gpio->port, in_gpio->pin);
        }

        err = gpio_pin_set(out_gpio->port, out_gpio->pin, 0);
        if (err) {
            LOG_ERR("Failed to set output %i inactive: %i", o, err);
            return err;
        }
    }

    // Process the new state.
#if USE_INTERRUPTS
    bool submit_followup_read = false;
#endif

    for (int r = 0; r < config->rows.len; r++) {
        for (int c = 0; c < config->cols.len; c++) {
            const int index = state_index_rc(config, r, c);
            const bool pressed = data->next_state[index];

            // Follow up reads are needed if any key is pressed because further
            // interrupts won't fire on already tripped GPIO pins.
#if USE_INTERRUPTS
            submit_followup_read = submit_followup_read || pressed;
#endif
            if (pressed != data->current_state[index]) {
                LOG_DBG("Sending event at %i,%i state %s", r, c, pressed ? "on" : "off");
                data->current_state[index] = pressed;
                data->callback(dev, r, c, pressed);
            }
        }
    }

#if USE_INTERRUPTS
    if (submit_followup_read) {
        // At least one key is pressed. Poll until everything is released.
        // TODO (Zephyr 2.6): use k_work_reschedule()
        k_delayed_work_cancel(&data->work);
        k_delayed_work_submit(&data->work, K_MSEC(config->debounce_period_ms));
    } else {
        // All keys are released. Return to waiting for an interrupt.
        kscan_matrix_interrupt_enable(dev);
    }
#endif

    return 0;
}

#if USE_POLLING
static void kscan_matrix_timer_handler(struct k_timer *timer) {
    struct kscan_matrix_data *data = CONTAINER_OF(timer, struct kscan_matrix_data, poll_timer);
    k_delayed_work_submit(&data->work, K_NO_WAIT);
}
#endif

static void kscan_matrix_work_handler(struct k_work *work) {
    struct k_delayed_work *dwork = CONTAINER_OF(work, struct k_delayed_work, work);
    struct kscan_matrix_data *data = CONTAINER_OF(dwork, struct kscan_matrix_data, work);
    kscan_matrix_read(data->dev);
}

static int kscan_matrix_configure(const struct device *dev, const kscan_callback_t callback) {
    struct kscan_matrix_data *data = dev->data;

    if (!callback) {
        return -EINVAL;
    }

    data->callback = callback;
    return 0;
}

static int kscan_matrix_enable(const struct device *dev) {
#if USE_POLLING
    struct kscan_matrix_data *data = dev->data;
    const struct kscan_matrix_config *config = dev->config;

    k_timer_start(&data->poll_timer, K_MSEC(config->poll_period_ms),
                  K_MSEC(config->poll_period_ms));
    return 0;
#else
    // Read will automatically enable interrupts once done.
    return kscan_matrix_read(dev);
#endif
}

static int kscan_matrix_disable(const struct device *dev) {
#if USE_POLLING
    struct kscan_matrix_data *data = dev->data;

    k_timer_stop(&data->poll_timer);
    return 0;
#else
    return kscan_matrix_interrupt_disable(dev);
#endif
}

static int kscan_matrix_init_input_inst(const struct device *dev,
                                        const struct kscan_gpio_dt_spec *gpio, const int index) {
    if (!device_is_ready(gpio->port)) {
        LOG_ERR("GPIO is not ready: %s", gpio->port->name);
        return -ENODEV;
    }

    int err = gpio_pin_configure(gpio->port, gpio->pin, GPIO_INPUT | gpio->dt_flags);
    if (err) {
        LOG_ERR("Unable to configure pin %u on %s for input", gpio->pin, gpio->port->name);
        return err;
    }

    LOG_DBG("Configured pin %u on %s for input", gpio->pin, gpio->port->name);

#if USE_INTERRUPTS
    struct kscan_matrix_data *data = dev->data;
    struct kscan_matrix_irq_callback *irq = &data->irqs[index];

    irq->dev = dev;
    irq->work = &data->work;
    gpio_init_callback(&irq->callback, kscan_matrix_irq_callback_handler, BIT(gpio->pin));
    err = gpio_add_callback(gpio->port, &irq->callback);
    if (err) {
        LOG_ERR("Error adding the callback to the input device: %i", err);
        return err;
    }
#endif

    return 0;
}

static int kscan_matrix_init_inputs(const struct device *dev) {
    const struct kscan_matrix_config *config = dev->config;

    for (int i = 0; i < config->inputs.len; i++) {
        const struct kscan_gpio_dt_spec *gpio = &config->inputs.gpios[i];
        int err = kscan_matrix_init_input_inst(dev, gpio, i);
        if (err) {
            return err;
        }
    }

    return 0;
}

static int kscan_matrix_init_output_inst(const struct device *dev,
                                         const struct kscan_gpio_dt_spec *gpio) {
    if (!device_is_ready(gpio->port)) {
        LOG_ERR("GPIO is not ready: %s", gpio->port->name);
        return -ENODEV;
    }

    int err = gpio_pin_configure(gpio->port, gpio->pin, GPIO_OUTPUT | gpio->dt_flags);
    if (err) {
        LOG_ERR("Unable to configure pin %u on %s for output", gpio->pin, gpio->port->name);
        return err;
    }

    LOG_DBG("Configured pin %u on %s for output", gpio->pin, gpio->port->name);

    return 0;
}

static int kscan_matrix_init_outputs(const struct device *dev) {
    const struct kscan_matrix_config *config = dev->config;

    for (int i = 0; i < config->outputs.len; i++) {
        const struct kscan_gpio_dt_spec *gpio = &config->outputs.gpios[i];
        int err = kscan_matrix_init_output_inst(dev, gpio);
        if (err) {
            return err;
        }
    }

    return 0;
}

static int kscan_matrix_init(const struct device *dev) {
    struct kscan_matrix_data *data = dev->data;

    data->dev = dev;

    kscan_matrix_init_inputs(dev);
    kscan_matrix_init_outputs(dev);
    kscan_matrix_set_all_outputs(dev, 0);

    k_delayed_work_init(&data->work, kscan_matrix_work_handler);

#if USE_POLLING
    k_timer_init(&data->poll_timer, kscan_matrix_timer_handler, NULL);
#endif

    return 0;
}

static const struct kscan_driver_api kscan_matrix_api = {
    .config = kscan_matrix_configure,
    .enable_callback = kscan_matrix_enable,
    .disable_callback = kscan_matrix_disable,
};

#define KSCAN_MATRIX_INIT(index)                                                                   \
    static const struct kscan_gpio_dt_spec kscan_matrix_rows_##index[] = {                         \
        UTIL_LISTIFY(INST_ROWS_LEN(index), KSCAN_GPIO_ROW_CFG_INIT, index)};                       \
                                                                                                   \
    static const struct kscan_gpio_dt_spec kscan_matrix_cols_##index[] = {                         \
        UTIL_LISTIFY(INST_COLS_LEN(index), KSCAN_GPIO_COL_CFG_INIT, index)};                       \
                                                                                                   \
    static bool kscan_current_state_##index[INST_MATRIX_LEN(index)];                               \
    static bool kscan_next_state_##index[INST_MATRIX_LEN(index)];                                  \
                                                                                                   \
    COND_INTERRUPTS((static struct kscan_matrix_irq_callback                                       \
                         kscan_matrix_irqs_##index[INST_INPUTS_LEN(index)];))                      \
                                                                                                   \
    static struct kscan_matrix_data kscan_matrix_data_##index = {                                  \
        .current_state = kscan_current_state_##index,                                              \
        .next_state = kscan_next_state_##index,                                                    \
        COND_INTERRUPTS((.irqs = kscan_matrix_irqs_##index, ))};                                   \
                                                                                                   \
    static struct kscan_matrix_config kscan_matrix_config_##index = {                              \
        .rows = KSCAN_GPIO_LIST(kscan_matrix_rows_##index),                                        \
        .cols = KSCAN_GPIO_LIST(kscan_matrix_cols_##index),                                        \
        .inputs = KSCAN_GPIO_LIST(                                                                 \
            COND_DIODE_DIR(index, (kscan_matrix_cols_##index), (kscan_matrix_rows_##index))),      \
        .outputs = KSCAN_GPIO_LIST(                                                                \
            COND_DIODE_DIR(index, (kscan_matrix_rows_##index), (kscan_matrix_cols_##index))),      \
        .debounce_period_ms = DT_INST_PROP(index, debounce_period),                                \
        .poll_period_ms = DT_INST_PROP(index, poll_period_ms),                                     \
        .diode_direction = INST_DIODE_DIR(index),                                                  \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(index, &kscan_matrix_init, device_pm_control_nop,                        \
                          &kscan_matrix_data_##index, &kscan_matrix_config_##index, APPLICATION,   \
                          CONFIG_APPLICATION_INIT_PRIORITY, &kscan_matrix_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_MATRIX_INIT);

#endif // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
