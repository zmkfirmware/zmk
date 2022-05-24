/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "debounce.h"

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/kscan.h>
#include <kernel.h>
#include <logging/log.h>
#include <sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_kscan_gpio_direct

#if CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS >= 0
#define INST_DEBOUNCE_PRESS_MS(n) CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS
#else
#define INST_DEBOUNCE_PRESS_MS(n)                                                                  \
    DT_INST_PROP_OR(n, debounce_period, DT_INST_PROP(n, debounce_press_ms))
#endif

#if CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS >= 0
#define INST_DEBOUNCE_RELEASE_MS(n) CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS
#else
#define INST_DEBOUNCE_RELEASE_MS(n)                                                                \
    DT_INST_PROP_OR(n, debounce_period, DT_INST_PROP(n, debounce_release_ms))
#endif

#define USE_POLLING IS_ENABLED(CONFIG_ZMK_KSCAN_DIRECT_POLLING)
#define USE_INTERRUPTS (!USE_POLLING)

#define COND_INTERRUPTS(code) COND_CODE_1(CONFIG_ZMK_KSCAN_DIRECT_POLLING, (), code)
#define COND_POLL_OR_INTERRUPTS(pollcode, intcode)                                                 \
    COND_CODE_1(CONFIG_ZMK_KSCAN_DIRECT_POLLING, pollcode, intcode)

#define INST_INPUTS_LEN(n) DT_INST_PROP_LEN(n, input_gpios)
#define KSCAN_DIRECT_INPUT_CFG_INIT(idx, inst_idx)                                                 \
    GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst_idx), input_gpios, idx),

struct kscan_direct_irq_callback {
    const struct device *dev;
    struct gpio_callback callback;
};

struct kscan_direct_data {
    const struct device *dev;
    kscan_callback_t callback;
    struct k_work_delayable work;
#if USE_INTERRUPTS
    /** Array of length config->inputs.len */
    struct kscan_direct_irq_callback *irqs;
#endif
    /** Timestamp of the current or scheduled scan. */
    int64_t scan_time;
    /** Current state of the inputs as an array of length config->inputs.len */
    struct debounce_state *pin_state;
};

struct kscan_gpio_list {
    const struct gpio_dt_spec *gpios;
    size_t len;
};

/** Define a kscan_gpio_list from a compile-time GPIO array. */
#define KSCAN_GPIO_LIST(gpio_array)                                                                \
    ((struct kscan_gpio_list){.gpios = gpio_array, .len = ARRAY_SIZE(gpio_array)})

struct kscan_direct_config {
    struct kscan_gpio_list inputs;
    struct debounce_config debounce_config;
    int32_t debounce_scan_period_ms;
    int32_t poll_period_ms;
    bool toggle_mode;
};

#if USE_INTERRUPTS
static int kscan_direct_interrupt_configure(const struct device *dev, const gpio_flags_t flags) {
    const struct kscan_direct_config *config = dev->config;

    for (int i = 0; i < config->inputs.len; i++) {
        const struct gpio_dt_spec *gpio = &config->inputs.gpios[i];

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
static int kscan_direct_interrupt_enable(const struct device *dev) {
    return kscan_direct_interrupt_configure(dev, GPIO_INT_LEVEL_ACTIVE);
}
#endif

#if USE_INTERRUPTS
static int kscan_direct_interrupt_disable(const struct device *dev) {
    return kscan_direct_interrupt_configure(dev, GPIO_INT_DISABLE);
}
#endif

#if USE_INTERRUPTS
static void kscan_direct_irq_callback_handler(const struct device *port, struct gpio_callback *cb,
                                              const gpio_port_pins_t pin) {
    struct kscan_direct_irq_callback *irq_data =
        CONTAINER_OF(cb, struct kscan_direct_irq_callback, callback);
    struct kscan_direct_data *data = irq_data->dev->data;

    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_direct_interrupt_disable(data->dev);

    data->scan_time = k_uptime_get();

    k_work_reschedule(&data->work, K_NO_WAIT);
}
#endif

static gpio_flags_t kscan_gpio_get_extra_flags(const struct gpio_dt_spec *gpio, bool active) {
    if (!active) {
        return ((BIT(0) & gpio->dt_flags) ? GPIO_PULL_UP : GPIO_PULL_DOWN);
    }
    return 0;
}

static int kscan_inputs_set_flags(const struct kscan_gpio_list *inputs,
                                  const struct gpio_dt_spec *active_gpio) {
    gpio_flags_t extra_flags;
    for (int i = 0; i < inputs->len; i++) {
        extra_flags = GPIO_INPUT | kscan_gpio_get_extra_flags(&inputs->gpios[i],
                                                              &inputs->gpios[i] == active_gpio);
        LOG_DBG("Extra flags equal to: %d", extra_flags);

        int err = gpio_pin_configure_dt(&inputs->gpios[i], extra_flags);
        if (err) {
            LOG_ERR("Unable to configure flags on pin %d on %s", inputs->gpios[i].pin,
                    inputs->gpios[i].port->name);
            return err;
        }
    }
    return 0;
}

static void kscan_direct_read_continue(const struct device *dev) {
    const struct kscan_direct_config *config = dev->config;
    struct kscan_direct_data *data = dev->data;

    data->scan_time += config->debounce_scan_period_ms;

    k_work_reschedule(&data->work, K_TIMEOUT_ABS_MS(data->scan_time));
}

static void kscan_direct_read_end(const struct device *dev) {
#if USE_INTERRUPTS
    // Return to waiting for an interrupt.
    kscan_direct_interrupt_enable(dev);
#else
    struct kscan_direct_data *data = dev->data;
    const struct kscan_direct_config *config = dev->config;

    data->scan_time += config->poll_period_ms;

    // Return to polling slowly.
    k_work_reschedule(&data->work, K_TIMEOUT_ABS_MS(data->scan_time));
#endif
}

static int kscan_direct_read(const struct device *dev) {
    struct kscan_direct_data *data = dev->data;
    const struct kscan_direct_config *config = dev->config;

    // Read the inputs.
    for (int i = 0; i < config->inputs.len; i++) {
        const struct gpio_dt_spec *gpio = &config->inputs.gpios[i];

        const bool active = gpio_pin_get_dt(gpio);

        debounce_update(&data->pin_state[i], active, config->debounce_scan_period_ms,
                        &config->debounce_config);
    }

    // Process the new state.
    bool continue_scan = false;

    for (int i = 0; i < config->inputs.len; i++) {
        struct debounce_state *state = &data->pin_state[i];

        if (debounce_get_changed(state)) {
            const bool pressed = debounce_is_pressed(state);

            LOG_DBG("Sending event at 0,%i state %s", i, pressed ? "on" : "off");
            data->callback(dev, 0, i, pressed);
            if (config->toggle_mode && pressed) {
                kscan_inputs_set_flags(&config->inputs, &config->inputs.gpios[i]);
            }
        }

        continue_scan = continue_scan || debounce_is_active(state);
    }

    if (continue_scan) {
        // At least one key is pressed or the debouncer has not yet decided if
        // it is pressed. Poll quickly until everything is released.
        kscan_direct_read_continue(dev);
    } else {
        // All keys are released. Return to normal.
        kscan_direct_read_end(dev);
    }

    return 0;
}

static void kscan_direct_work_handler(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct kscan_direct_data *data = CONTAINER_OF(dwork, struct kscan_direct_data, work);
    kscan_direct_read(data->dev);
}

static int kscan_direct_configure(const struct device *dev, kscan_callback_t callback) {
    struct kscan_direct_data *data = dev->data;

    if (!callback) {
        return -EINVAL;
    }

    data->callback = callback;
    return 0;
}

static int kscan_direct_enable(const struct device *dev) {
    struct kscan_direct_data *data = dev->data;

    data->scan_time = k_uptime_get();

    // Read will automatically start interrupts/polling once done.
    return kscan_direct_read(dev);
}

static int kscan_direct_disable(const struct device *dev) {
    struct kscan_direct_data *data = dev->data;

    k_work_cancel_delayable(&data->work);

#if USE_INTERRUPTS
    return kscan_direct_interrupt_disable(dev);
#else
    return 0;
#endif
}

static int kscan_direct_init_input_inst(const struct device *dev, const struct gpio_dt_spec *gpio,
                                        const int index, bool toggle_mode) {
    if (!device_is_ready(gpio->port)) {
        LOG_ERR("GPIO is not ready: %s", gpio->port->name);
        return -ENODEV;
    }
    int err = gpio_pin_configure_dt(
        gpio, GPIO_INPUT | (toggle_mode ? kscan_gpio_get_extra_flags(gpio, false) : 0));
    if (err) {
        LOG_ERR("Unable to configure pin %u on %s for input", gpio->pin, gpio->port->name);
        return err;
    }

    LOG_DBG("Configured pin %u on %s for input", gpio->pin, gpio->port->name);

#if USE_INTERRUPTS
    struct kscan_direct_data *data = dev->data;
    struct kscan_direct_irq_callback *irq = &data->irqs[index];

    irq->dev = dev;
    gpio_init_callback(&irq->callback, kscan_direct_irq_callback_handler, BIT(gpio->pin));
    err = gpio_add_callback(gpio->port, &irq->callback);
    if (err) {
        LOG_ERR("Error adding the callback to the input device: %i", err);
        return err;
    }
#endif

    return 0;
}

static int kscan_direct_init_inputs(const struct device *dev) {
    const struct kscan_direct_config *config = dev->config;

    for (int i = 0; i < config->inputs.len; i++) {
        const struct gpio_dt_spec *gpio = &config->inputs.gpios[i];
        int err = kscan_direct_init_input_inst(dev, gpio, i, config->toggle_mode);
        if (err) {
            return err;
        }
    }

    return 0;
}

static int kscan_direct_init(const struct device *dev) {
    struct kscan_direct_data *data = dev->data;

    data->dev = dev;

    kscan_direct_init_inputs(dev);

    k_work_init_delayable(&data->work, kscan_direct_work_handler);

    return 0;
}

static const struct kscan_driver_api kscan_direct_api = {
    .config = kscan_direct_configure,
    .enable_callback = kscan_direct_enable,
    .disable_callback = kscan_direct_disable,
};

#define KSCAN_DIRECT_INIT(n)                                                                       \
    BUILD_ASSERT(INST_DEBOUNCE_PRESS_MS(n) <= DEBOUNCE_COUNTER_MAX,                                \
                 "ZMK_KSCAN_DEBOUNCE_PRESS_MS or debounce-press-ms is too large");                 \
    BUILD_ASSERT(INST_DEBOUNCE_RELEASE_MS(n) <= DEBOUNCE_COUNTER_MAX,                              \
                 "ZMK_KSCAN_DEBOUNCE_RELEASE_MS or debounce-release-ms is too large");             \
                                                                                                   \
    static const struct gpio_dt_spec kscan_direct_inputs_##n[] = {                                 \
        UTIL_LISTIFY(INST_INPUTS_LEN(n), KSCAN_DIRECT_INPUT_CFG_INIT, n)};                         \
                                                                                                   \
    static struct debounce_state kscan_direct_state_##n[INST_INPUTS_LEN(n)];                       \
                                                                                                   \
    COND_INTERRUPTS(                                                                               \
        (static struct kscan_direct_irq_callback kscan_direct_irqs_##n[INST_INPUTS_LEN(n)];))      \
                                                                                                   \
    static struct kscan_direct_data kscan_direct_data_##n = {                                      \
        .pin_state = kscan_direct_state_##n, COND_INTERRUPTS((.irqs = kscan_direct_irqs_##n, ))};  \
                                                                                                   \
    static struct kscan_direct_config kscan_direct_config_##n = {                                  \
        .inputs = KSCAN_GPIO_LIST(kscan_direct_inputs_##n),                                        \
        .debounce_config =                                                                         \
            {                                                                                      \
                .debounce_press_ms = INST_DEBOUNCE_PRESS_MS(n),                                    \
                .debounce_release_ms = INST_DEBOUNCE_RELEASE_MS(n),                                \
            },                                                                                     \
        .debounce_scan_period_ms = DT_INST_PROP(n, debounce_scan_period_ms),                       \
        .poll_period_ms = DT_INST_PROP(n, poll_period_ms),                                         \
        .toggle_mode = DT_INST_PROP(n, toggle_mode),                                               \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, &kscan_direct_init, NULL, &kscan_direct_data_##n,                     \
                          &kscan_direct_config_##n, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY, \
                          &kscan_direct_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_DIRECT_INIT);
