/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_scaler

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct scaler_config {
    uint8_t type;
    size_t codes_len;
    uint16_t codes[];
};

static int scale_val(struct input_event *event, uint32_t mul, uint32_t div,
                     struct zmk_input_processor_state *state) {
    float value = ((((float)event->value) * mul) / div);
    if (state && state->remainder) {
        value += *state->remainder;
    }

    event->value = (int16_t)value;
    LOG_DBG("SCALED VALUE %f, cast to int %d", value, event->value);

    if (state && state->remainder) {
        *state->remainder = value - event->value;
    }

    return 0;
}

static int scaler_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state) {
    const struct scaler_config *cfg = dev->config;

    if (event->type != cfg->type) {
        return 0;
    }

    for (int i = 0; i < cfg->codes_len; i++) {
        if (cfg->codes[i] == event->code) {
            return scale_val(event, param1, param2, state);
        }
    }

    return 0;
}

static struct zmk_input_processor_driver_api scaler_driver_api = {
    .handle_event = scaler_handle_event,
};

#define SCALER_INST(n)                                                                             \
    static const struct scaler_config scaler_config_##n = {                                        \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                                   \
        .codes = DT_INST_PROP(n, codes),                                                           \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &scaler_config_##n, POST_KERNEL,                    \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &scaler_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCALER_INST)