/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_gpio_direct

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

union work_reference {
    struct k_delayed_work delayed;
    struct k_work direct;
};

struct kscan_gpio_config {
    u8_t num_of_inputs;
    u8_t debounce_period;
    struct kscan_gpio_item_config inputs[];
};

struct kscan_gpio_data {
#if defined(CONFIG_ZMK_KSCAN_DIRECT_POLLING)
    struct k_timer poll_timer;
#endif /* defined(CONFIG_ZMK_KSCAN_DIRECT_POLLING) */
    kscan_callback_t callback;
    union work_reference work;
    struct device *dev;
    u32_t pin_state;
    struct device *inputs[];
};

static struct device **kscan_gpio_input_devices(struct device *dev) {
    struct kscan_gpio_data *data = dev->driver_data;
    return data->inputs;
}

static const struct kscan_gpio_item_config *kscan_gpio_input_configs(struct device *dev) {
    const struct kscan_gpio_config *cfg = dev->config_info;
    return cfg->inputs;
}

#if !defined(CONFIG_ZMK_KSCAN_DIRECT_POLLING)

struct kscan_gpio_irq_callback {
    union work_reference *work;
    u8_t debounce_period;
    struct gpio_callback callback;
};

static int kscan_gpio_config_interrupts(struct device *dev, gpio_flags_t flags) {
    const struct kscan_gpio_config *cfg = dev->config_info;
    struct device **devices = kscan_gpio_input_devices(dev);
    const struct kscan_gpio_item_config *configs = kscan_gpio_input_configs(dev);

    for (int i = 0; i < cfg->num_of_inputs; i++) {
        struct device *dev = devices[i];
        const struct kscan_gpio_item_config *cfg = &configs[i];

        int err = gpio_pin_interrupt_configure(dev, cfg->pin, flags);

        if (err) {
            LOG_ERR("Unable to enable matrix GPIO interrupt");
            return err;
        }
    }

    return 0;
}

static int kscan_gpio_direct_enable(struct device *dev) {
    return kscan_gpio_config_interrupts(dev, GPIO_INT_DEBOUNCE | GPIO_INT_EDGE_BOTH);
}
static int kscan_gpio_direct_disable(struct device *dev) {
    return kscan_gpio_config_interrupts(dev, GPIO_INT_DISABLE);
}

static void kscan_gpio_irq_callback_handler(struct device *dev, struct gpio_callback *cb,
                                            gpio_port_pins_t pin) {
    struct kscan_gpio_irq_callback *data =
        CONTAINER_OF(cb, struct kscan_gpio_irq_callback, callback);

    if (data->debounce_period > 0) {
        k_delayed_work_cancel(&data->work->delayed);
        k_delayed_work_submit(&data->work->delayed, K_MSEC(data->debounce_period));
    } else {
        k_work_submit(&data->work->direct);
    }
}

#else /* !defined(CONFIG_ZMK_KSCAN_DIRECT_POLLING) */

static void kscan_gpio_timer_handler(struct k_timer *timer) {
    struct kscan_gpio_data *data = CONTAINER_OF(timer, struct kscan_gpio_data, poll_timer);

    k_work_submit(&data->work.direct);
}

static int kscan_gpio_direct_enable(struct device *dev) {
    struct kscan_gpio_data *data = dev->driver_data;
    k_timer_start(&data->poll_timer, K_MSEC(10), K_MSEC(10));
    return 0;
}
static int kscan_gpio_direct_disable(struct device *dev) {
    struct kscan_gpio_data *data = dev->driver_data;
    k_timer_stop(&data->poll_timer);
    return 0;
}

#endif /* defined(CONFIG_ZMK_KSCAN_DIRECT_POLLING) */

static int kscan_gpio_direct_configure(struct device *dev, kscan_callback_t callback) {
    struct kscan_gpio_data *data = dev->driver_data;
    if (!callback) {
        return -EINVAL;
    }
    data->callback = callback;
    return 0;
}

static int kscan_gpio_read(struct device *dev) {
    struct kscan_gpio_data *data = dev->driver_data;
    const struct kscan_gpio_config *cfg = dev->config_info;
    u32_t read_state = data->pin_state;
    for (int i = 0; i < cfg->num_of_inputs; i++) {
        struct device *in_dev = kscan_gpio_input_devices(dev)[i];
        const struct kscan_gpio_item_config *in_cfg = &kscan_gpio_input_configs(dev)[i];
        WRITE_BIT(read_state, i, gpio_pin_get(in_dev, in_cfg->pin) > 0);
    }
    for (int i = 0; i < cfg->num_of_inputs; i++) {
        bool prev_pressed = BIT(i) & data->pin_state;
        bool pressed = BIT(i) & read_state;
        if (pressed != prev_pressed) {
            LOG_DBG("Sending event at %d,%d state %s", 0, i, (pressed ? "on" : "off"));
            WRITE_BIT(data->pin_state, i, pressed);
            data->callback(dev, 0, i, pressed);
        }
    }
    return 0;
}

static void kscan_gpio_work_handler(struct k_work *work) {
    struct kscan_gpio_data *data = CONTAINER_OF(work, struct kscan_gpio_data, work);
    kscan_gpio_read(data->dev);
}

static const struct kscan_driver_api gpio_driver_api = {
    .config = kscan_gpio_direct_configure,
    .enable_callback = kscan_gpio_direct_enable,
    .disable_callback = kscan_gpio_direct_disable,
};

#define KSCAN_DIRECT_INPUT_ITEM(i, n)                                                              \
    {                                                                                              \
        .label = DT_INST_GPIO_LABEL_BY_IDX(n, input_gpios, i),                                     \
        .pin = DT_INST_GPIO_PIN_BY_IDX(n, input_gpios, i),                                         \
        .flags = DT_INST_GPIO_FLAGS_BY_IDX(n, input_gpios, i),                                     \
    },

#define INST_INPUT_LEN(n) DT_INST_PROP_LEN(n, input_gpios)

#define GPIO_INST_INIT(n)                                                                          \
    COND_CODE_0(CONFIG_ZMK_KSCAN_DIRECT_POLLING,                                                   \
                (static struct kscan_gpio_irq_callback irq_callbacks_##n[INST_INPUT_LEN(n)];), ()) \
    static struct kscan_gpio_data kscan_gpio_data_##n = {                                          \
        .inputs = {[INST_INPUT_LEN(n) - 1] = NULL}};                                               \
    static int kscan_gpio_init_##n(struct device *dev) {                                           \
        struct kscan_gpio_data *data = dev->driver_data;                                           \
        const struct kscan_gpio_config *cfg = dev->config_info;                                    \
        int err;                                                                                   \
        struct device **input_devices = kscan_gpio_input_devices(dev);                             \
        for (int i = 0; i < cfg->num_of_inputs; i++) {                                             \
            const struct kscan_gpio_item_config *in_cfg = &kscan_gpio_input_configs(dev)[i];       \
            input_devices[i] = device_get_binding(in_cfg->label);                                  \
            if (!input_devices[i]) {                                                               \
                LOG_ERR("Unable to find input GPIO device");                                       \
                return -EINVAL;                                                                    \
            }                                                                                      \
            err = gpio_pin_configure(input_devices[i], in_cfg->pin, GPIO_INPUT | in_cfg->flags);   \
            if (err) {                                                                             \
                LOG_ERR("Unable to configure pin %d on %s for input", in_cfg->pin, in_cfg->label); \
                return err;                                                                        \
            }                                                                                      \
            COND_CODE_0(                                                                           \
                CONFIG_ZMK_KSCAN_DIRECT_POLLING,                                                   \
                (irq_callbacks_##n[i].work = &data->work;                                          \
                 irq_callbacks_##n[i].debounce_period = cfg->debounce_period;                      \
                 gpio_init_callback(&irq_callbacks_##n[i].callback,                                \
                                    kscan_gpio_irq_callback_handler, BIT(in_cfg->pin));            \
                 err = gpio_add_callback(input_devices[i], &irq_callbacks_##n[i].callback);        \
                 if (err) {                                                                        \
                     LOG_ERR("Error adding the callback to the column device");                    \
                     return err;                                                                   \
                 }),                                                                               \
                ())                                                                                \
        }                                                                                          \
        data->dev = dev;                                                                           \
        COND_CODE_1(CONFIG_ZMK_KSCAN_DIRECT_POLLING,                                               \
                    (k_timer_init(&data->poll_timer, kscan_gpio_timer_handler, NULL);), ())        \
        if (cfg->debounce_period > 0) {                                                            \
            k_delayed_work_init(&data->work.delayed, kscan_gpio_work_handler);                     \
        } else {                                                                                   \
            k_work_init(&data->work.direct, kscan_gpio_work_handler);                              \
        }                                                                                          \
        return 0;                                                                                  \
    }                                                                                              \
    static const struct kscan_gpio_config kscan_gpio_config_##n = {                                \
        .inputs = {UTIL_LISTIFY(INST_INPUT_LEN(n), KSCAN_DIRECT_INPUT_ITEM, n)},                   \
        .num_of_inputs = INST_INPUT_LEN(n),                                                        \
        .debounce_period = DT_INST_PROP(n, debounce_period)};                                      \
    DEVICE_AND_API_INIT(kscan_gpio_##n, DT_INST_LABEL(n), kscan_gpio_init_##n,                     \
                        &kscan_gpio_data_##n, &kscan_gpio_config_##n, POST_KERNEL,                 \
                        CONFIG_ZMK_KSCAN_INIT_PRIORITY, &gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_INST_INIT)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
