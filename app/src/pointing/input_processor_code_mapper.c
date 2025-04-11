/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_code_mapper

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct cm_config {
    uint8_t type;
    size_t mapping_size;
    uint16_t mapping[];
};

static int cm_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                           uint32_t param2, struct zmk_input_processor_state *state) {
    const struct cm_config *cfg = dev->config;

    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    for (int i = 0; i < cfg->mapping_size / 2; i++) {
        if (cfg->mapping[i * 2] == event->code) {
            uint16_t orig = event->code;
            event->code = cfg->mapping[(i * 2) + 1];
            LOG_DBG("Remapped %d to %d", orig, event->code);
            break;
        }
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api cm_driver_api = {
    .handle_event = cm_handle_event,
};

#define TL_INST(n)                                                                                 \
    static const struct cm_config cm_config_##n = {                                                \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .mapping_size = DT_INST_PROP_LEN(n, map),                                                  \
        .mapping = DT_INST_PROP(n, map),                                                           \
    };                                                                                             \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, map) % 2 == 0,                                                \
                 "Must have an even number of mapping entries");                                   \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &cm_config_##n, POST_KERNEL,                        \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &cm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TL_INST)