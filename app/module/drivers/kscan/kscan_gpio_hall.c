/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <stdlib.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_kscan_gpio_hall

#define EMA_SHIFT 4

#define INST_INPUTS_LEN(n) DT_INST_PROP_LEN(n, io_channels)

#define KSCAN_ADC_GET_BY_IDX(node_id, idx)                                                         \
    ((struct kscan_adc){.spec = ADC_DT_SPEC_GET_BY_IDX(node_id, idx), .index = idx})

#define KSCAN_ADC_HALL_INPUT_CFG_INIT(idx, inst_idx)                                               \
    KSCAN_ADC_GET_BY_IDX(DT_DRV_INST(inst_idx), idx)

#define KSCAN_ADC_LIST(adc_array)                                                                  \
    ((struct kscan_adc_list){.adcs = adc_array, .len = ARRAY_SIZE(adc_array)})

struct kscan_adc {
    struct adc_dt_spec spec;
    struct adc_channel_cfg cfg;
    struct adc_sequence as;
    size_t index;
};

struct kscan_adc_list {
    struct kscan_adc *adcs;
    size_t len;
};

struct kscan_key_state {
    int last_value;
    int idle_value;
    int max_value;
    int min_value;
    int range;
    bool pressed : 1;
};

struct kscan_hall_data {
    const struct device *dev;
    struct kscan_adc_list inputs;
    kscan_callback_t callback;
    struct k_work_delayable work;
    /** Timestamp of the current or scheduled scan. */
    int64_t scan_time;
    /** Current state of the inputs as an array of length config->inputs.len */
    struct kscan_key_state *state;
};

struct kscan_hall_config {
    int min_trigger_value;
    int max_trigger_value;
    int trigger_value;
    int init_range;
    int idle_steps;
};

static int16_t current_value;

int kscan_adc_read(const struct kscan_adc *adc) {
    int err = adc_read(adc->spec.dev, &adc->as);
    if (err != 0) {
        LOG_ERR("Failed to read %s, channel: %i", adc->spec.dev->name, adc->spec.channel_id);
        return err;
    }

    return 0;
}

static int kscan_hall_read(const struct device *dev) {
    struct kscan_hall_data *data = dev->data;
    const struct kscan_hall_config *config = dev->config;

    for (int i = 0; i < data->inputs.len; i++) {
        struct kscan_adc *adc = &data->inputs.adcs[i];

        const int read_err = kscan_adc_read(adc);
        if (read_err != 0) {
            return read_err;
        }

        struct kscan_key_state *state = &data->state[adc->index];

        if (state->max_value == 0) {
            state->max_value = state->min_value = state->last_value = current_value + 1;
            continue;
        }

        state->last_value += (current_value - state->last_value) >> EMA_SHIFT;

        if (state->max_value < state->last_value) {
            state->max_value = state->last_value + 1;

            int t = state->max_value - state->idle_value;
            if (state->range < t)
                state->range = t;
        }

        if (state->min_value > state->last_value) {
            state->min_value = state->last_value - 1;

            int t = abs(state->min_value - state->idle_value);
            if (state->range < t)
                state->range = t;
        }

        int value = (abs(state->last_value - state->idle_value) / (float)state->range) * 100;

        if (value >= config->max_trigger_value) {
            continue;
        }

        if (value <= config->min_trigger_value) {
            if (state->pressed) {
                state->pressed = false;
                LOG_DBG("Sending event at 0,%i state %s, value %i", adc->index,
                        state->pressed ? "on" : "off", value);
                data->callback(dev, 0, adc->index, state->pressed);
            }
            continue;
        }

        bool key_state = state->pressed;

        // Press
        if (!state->pressed && value >= config->trigger_value) {
            key_state = true;
        }

        if (state->pressed != key_state) {
            state->pressed = key_state;
            LOG_DBG("Sending event at 0,%i state %s, value %i.", adc->index,
                    state->pressed ? "on" : "off", value);
            data->callback(dev, 0, adc->index, state->pressed);
        }
    }

    k_work_reschedule(&data->work, K_TIMEOUT_ABS_MS(data->scan_time));
    return 0;
}

static int kscan_hall_init_input_inst(struct kscan_adc *adc) {
    struct adc_dt_spec *spec = &adc->spec;

    // TODO: check spec->channel_cfg_dt_node_exists
    if (!device_is_ready(spec->dev)) {
        LOG_ERR("ADC is not ready: %s", spec->dev->name);
        return -ENODEV;
    }

#ifdef CONFIG_ADC_NRFX_SAADC
    adc->cfg = (struct adc_channel_cfg){
        .channel_id = spec->channel_id,
        .reference = ADC_REF_INTERNAL,
        .gain = ADC_GAIN_1_6,
        // TODO: adjust relative to the power mode
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + spec->channel_id,
    };
#endif

    // TODO: channel_cfg_dt_node_exists

    int err = adc_channel_setup(spec->dev, &adc->cfg);
    if (err != 0) {
        LOG_ERR("Unable to configure channel %u of device %s. Err: %i", spec->channel_id,
                spec->dev->name, err);
        return err;
    }

    // TODO: create per device (adc1, adc2, MCP3208, etc)
    adc->as = (struct adc_sequence){
        .channels = BIT(spec->channel_id),
        .buffer = &current_value,
        .buffer_size = sizeof(current_value),
        .oversampling = 2,
        .calibrate = true,
        .resolution = 12,
    };

    LOG_DBG("%s: AIN%u setup returned %d", spec->dev->name, spec->channel_id, err);
    return 0;
}

static int kscan_hall_init_idle_value(const struct kscan_adc *adc, struct kscan_key_state *state,
                                      int steps) {
    int32_t sum = 1;

    for (int i = 0; i < steps; i++) {
        int err = kscan_adc_read(adc);
        if (err) {
            return err;
        }
        sum += current_value;
        k_sleep(K_MSEC(5));
    }

    state->idle_value = sum / steps;
    return 0;
}

static int kscan_hall_init_inputs(const struct device *dev) {
    const struct kscan_hall_data *data = dev->data;
    const struct kscan_hall_config *config = dev->config;

    for (int i = 0; i < data->inputs.len; i++) {
        struct kscan_adc *adc = &data->inputs.adcs[i];
        struct kscan_key_state *state = &data->state[i];

        int err = kscan_hall_init_input_inst(adc);
        if (err) {
            return err;
        }

        err = kscan_hall_init_idle_value(adc, state, config->idle_steps);
        if (err) {
            return err;
        }

        // Disable calibration. No further need.
        adc->as.calibrate = false;

        state->range = config->init_range;
    }
    return 0;
}

static void kscan_hall_work_handler(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct kscan_hall_data *data = CONTAINER_OF(dwork, struct kscan_hall_data, work);
    kscan_hall_read(data->dev);
}

static int compare_ports2(const void *a, const void *b) {
    const struct kscan_adc *adc_a = a;
    const struct kscan_adc *adc_b = b;

    return (adc_a->spec.dev + adc_a->spec.channel_id) - (adc_b->spec.dev + adc_b->spec.channel_id);
}

void kscan_adc_list_sort_by_port(struct kscan_adc_list *list) {
    qsort(list->adcs, list->len, sizeof(list->adcs[0]), compare_ports2);
}

static int kscan_hall_init(const struct device *dev) {
    struct kscan_hall_data *data = dev->data;

    data->dev = dev;

    // Sort inputs by port so we can read each port just once per scan.
    kscan_adc_list_sort_by_port(&data->inputs);

    k_work_init_delayable(&data->work, kscan_hall_work_handler);

    kscan_hall_init_inputs(dev);
    return 0;
}

static int kscan_hall_configure(const struct device *dev, kscan_callback_t callback) {
    struct kscan_hall_data *data = dev->data;

    if (!callback) {
        return -EINVAL;
    }

    data->callback = callback;
    return 0;
}

static int kscan_hall_enable(const struct device *dev) {
    struct kscan_hall_data *data = dev->data;
    data->scan_time = k_uptime_get();
    return kscan_hall_read(dev);
}

static int kscan_hall_disable(const struct device *dev) {
    struct kscan_hall_data *data = dev->data;
    k_work_cancel_delayable(&data->work);
    return 0;
}

static const struct kscan_driver_api kscan_hall_api = {
    .config = kscan_hall_configure,
    .enable_callback = kscan_hall_enable,
    .disable_callback = kscan_hall_disable,
};

#define KSCAN_HALL_INIT(n)                                                                         \
    static struct kscan_adc kscan_hall_inputs_##n[] = {                                            \
        LISTIFY(INST_INPUTS_LEN(n), KSCAN_ADC_HALL_INPUT_CFG_INIT, (, ), n)};                      \
                                                                                                   \
    static struct kscan_key_state kscan_hall_state_##n[INST_INPUTS_LEN(n)];                        \
                                                                                                   \
    static struct kscan_hall_data kscan_hall_data_##n = {                                          \
        .inputs = KSCAN_ADC_LIST(kscan_hall_inputs_##n),                                           \
        .state = kscan_hall_state_##n,                                                             \
    };                                                                                             \
                                                                                                   \
    static const struct kscan_hall_config kscan_hall_config_##n = {                                \
        .min_trigger_value = DT_INST_PROP(n, min_trigger_value),                                   \
        .max_trigger_value = DT_INST_PROP(n, max_trigger_value),                                   \
        .trigger_value = DT_INST_PROP(n, trigger_value),                                           \
        .init_range = DT_INST_PROP(n, init_range),                                                 \
        .idle_steps = DT_INST_PROP(n, idle_steps),                             \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, &kscan_hall_init, PM_DEVICE_DT_INST_GET(n), &kscan_hall_data_##n,     \
                          &kscan_hall_config_##n, POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,         \
                          &kscan_hall_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_HALL_INIT);
