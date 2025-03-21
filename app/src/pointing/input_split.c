/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_split

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

struct zis_entry {
    uint8_t reg;
    const struct device *dev;
};

#define ZIS_ENTRY(n) {.reg = DT_INST_REG_ADDR(n), .dev = DEVICE_DT_GET(DT_DRV_INST(n))},

static const struct zis_entry proxy_inputs[] = {DT_INST_FOREACH_STATUS_OKAY(ZIS_ENTRY)};

#define PLUS_ONE(n) +1

static uint16_t proxy_active_keys[(0 DT_INST_FOREACH_STATUS_OKAY(PLUS_ONE))]
                                 [CONFIG_ZMK_INPUT_SPLIT_MAX_TRACKED_KEYS];

static int replace_active_key(size_t input, uint16_t find, uint16_t replace) {
    for (size_t k = 0; k < CONFIG_ZMK_INPUT_SPLIT_MAX_TRACKED_KEYS; k++) {
        if (proxy_active_keys[input][k] == find) {
            proxy_active_keys[input][k] = replace;
            return 0;
        }
    }

    return -ENODEV;
}

int zmk_input_split_report_peripheral_event(uint8_t reg, uint8_t type, uint16_t code, int32_t value,
                                            bool sync) {
    LOG_DBG("Got peripheral event for %d!", reg);
    for (size_t i = 0; i < ARRAY_SIZE(proxy_inputs); i++) {
        if (reg == proxy_inputs[i].reg) {
            if (type == INPUT_EV_KEY) {
                if (value) {
                    int ret = replace_active_key(i, 0, code);
                    if (ret < 0) {
                        LOG_WRN("Failed to track pressed key %d", ret);
                    }
                } else {
                    int ret = replace_active_key(i, code, 0);
                    if (ret < 0) {
                        LOG_WRN("Failed to untrack released key %d", ret);
                    }
                }
            }
            return input_report(proxy_inputs[i].dev, type, code, value, sync, K_NO_WAIT);
        }
    }

    return -ENODEV;
}

int zmk_input_split_peripheral_disconnected(uint8_t reg) {
    for (size_t i = 0; i < ARRAY_SIZE(proxy_inputs); i++) {
        if (reg == proxy_inputs[i].reg) {
            uint16_t prev = 0;
            for (size_t k = 0; k < CONFIG_ZMK_INPUT_SPLIT_MAX_TRACKED_KEYS; k++) {
                if (proxy_active_keys[i][k] != 0) {
                    if (prev != 0) {
                        int ret = input_report(proxy_inputs[i].dev, INPUT_EV_KEY, prev, 0, false,
                                               K_NO_WAIT);
                        if (ret < 0) {
                            LOG_WRN("Failed to report release event on disconnect (%d)", ret);
                        }
                    }

                    prev = proxy_active_keys[i][k];
                    proxy_active_keys[i][k] = 0;
                }
            }

            if (prev != 0) {
                int ret = input_report(proxy_inputs[i].dev, INPUT_EV_KEY, prev, 0, true, K_NO_WAIT);
                if (ret < 0) {
                    LOG_WRN("Failed to report release event on disconnect (%d)", ret);
                }
            }
        }
    }

    return -ENODEV;
}

#define ZIS_INST(n)                                                                                \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                  \
                          CONFIG_ZMK_INPUT_SPLIT_INIT_PRIORITY, NULL);

#else

#include <zmk/split/peripheral.h>

#define ZIS_INST(n)                                                                                \
    static const struct zmk_input_processor_entry processors_##n[] =                               \
        COND_CODE_1(DT_INST_NODE_HAS_PROP(n, input_processors),                                    \
                    ({LISTIFY(DT_INST_PROP_LEN(n, input_processors),                               \
                              ZMK_INPUT_PROCESSOR_ENTRY_AT_IDX, (, ), DT_DRV_INST(n))}),           \
                    ({}));                                                                         \
    BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, device),                                                 \
                 "Peripheral input splits need an `input` property set");                          \
    void split_input_handler_##n(struct input_event *evt) {                                        \
        for (size_t i = 0; i < ARRAY_SIZE(processors_##n); i++) {                                  \
            zmk_input_processor_handle_event(processors_##n[i].dev, evt, processors_##n[i].param1, \
                                             processors_##n[i].param2, NULL);                      \
        }                                                                                          \
        struct zmk_split_transport_peripheral_event ev = {                                         \
            .type = ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_INPUT_EVENT,                         \
            .data = {.input_event = {                                                              \
                         .reg = DT_INST_REG_ADDR(n),                                               \
                         .type = evt->type,                                                        \
                         .code = evt->code,                                                        \
                         .value = evt->value,                                                      \
                         .sync = evt->sync,                                                        \
                     }}};                                                                          \
        zmk_split_peripheral_report_event(&ev);                                                    \
    }                                                                                              \
    INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_INST_PHANDLE(n, device)), split_input_handler_##n);

#endif

DT_INST_FOREACH_STATUS_OKAY(ZIS_INST)