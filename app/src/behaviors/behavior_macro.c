/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>

#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/hid.h>

#define DT_DRV_COMPAT zmk_behavior_macro

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_macro_config {
    int sleep;
    int behavior_count;
    struct zmk_behavior_binding *behaviors;
};

static int behavior_macro_init(const struct device *dev) { return 0; }

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_macro_config *cfg = dev->config;

    for (int index = 0; index < cfg->behavior_count; index++) {
        const struct device *behavior = device_get_binding(cfg->behaviors[index].behavior_dev);
        if (!behavior) {
            break;
        }
        behavior_keymap_binding_pressed(&cfg->behaviors[index], event);
        k_msleep(cfg->sleep);
        behavior_keymap_binding_released(&cfg->behaviors[index], event);
        if (index + 1 < cfg->behavior_count) {
            k_msleep(cfg->sleep);
        }
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_macro_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    },

#define TRANSFORMED_BINDINGS(node)                                                                 \
    { UTIL_LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_ENTRY, node) }

#define KP_INST(n)                                                                                 \
    static struct zmk_behavior_binding                                                             \
        behavior_macro_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =                      \
            TRANSFORMED_BINDINGS(n);                                                               \
    static struct behavior_macro_config behavior_macro_config_##n = {                              \
        .behaviors = behavior_macro_config_##n##_bindings,                                         \
        .behavior_count = DT_INST_PROP_LEN(n, bindings),                                           \
        .sleep = DT_INST_PROP(n, sleep),                                                           \
    };                                                                                             \
    DEVICE_AND_API_INIT(behavior_macro_##n, DT_INST_LABEL(n), behavior_macro_init, NULL,           \
                        &behavior_macro_config_##n, APPLICATION,                                   \
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_macro_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)
#endif