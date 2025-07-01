/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_transform

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <dt-bindings/zmk/input_transform.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>

struct ipt_config {
    size_t x_codes_size;
    size_t y_codes_size;
    uint8_t type;

    const uint16_t *x_codes;
    const uint16_t *y_codes;
};

static int code_idx(uint16_t code, const uint16_t *list, size_t len) {
    for (int i = 0; i < len; i++) {
        if (list[i] == code) {
            return i;
        }
    }

    return -ENODEV;
}

static int ipt_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                            uint32_t param2, struct zmk_input_processor_state *state) {
    const struct ipt_config *cfg = dev->config;

    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (param1 & INPUT_TRANSFORM_XY_SWAP) {
        int idx = code_idx(event->code, cfg->x_codes, cfg->x_codes_size);
        if (idx >= 0) {
            event->code = cfg->y_codes[idx];
        } else {
            idx = code_idx(event->code, cfg->y_codes, cfg->y_codes_size);

            if (idx >= 0) {
                event->code = cfg->x_codes[idx];
            }
        }
    }

    if ((param1 & INPUT_TRANSFORM_X_INVERT &&
         code_idx(event->code, cfg->x_codes, cfg->x_codes_size) >= 0) ||
        (param1 & INPUT_TRANSFORM_Y_INVERT &&
         code_idx(event->code, cfg->y_codes, cfg->y_codes_size) >= 0)) {
        event->value = -event->value;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api ipt_driver_api = {
    .handle_event = ipt_handle_event,
};

static int ipt_init(const struct device *dev) { return 0; }

#define IPT_INST(n)                                                                                \
    static const uint16_t ipt_x_codes_##n[] = DT_INST_PROP(n, x_codes);                            \
    static const uint16_t ipt_y_codes_##n[] = DT_INST_PROP(n, y_codes);                            \
    BUILD_ASSERT(ARRAY_SIZE(ipt_x_codes_##n) == ARRAY_SIZE(ipt_x_codes_##n),                       \
                 "X and Y codes need to be the same size");                                        \
    static const struct ipt_config ipt_config_##n = {                                              \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .x_codes_size = DT_INST_PROP_LEN(n, x_codes),                                              \
        .y_codes_size = DT_INST_PROP_LEN(n, y_codes),                                              \
        .x_codes = ipt_x_codes_##n,                                                                \
        .y_codes = ipt_y_codes_##n,                                                                \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, &ipt_init, NULL, NULL, &ipt_config_##n, POST_KERNEL,                  \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ipt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IPT_INST)