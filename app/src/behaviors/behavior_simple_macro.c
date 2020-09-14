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
#include <zmk/event-manager.h>
#include <zmk/events/keycode-state-changed.h>
#include <zmk/events/modifiers-state-changed.h>
#include <zmk/hid.h>

#define DT_DRV_COMPAT zmk_behavior_simple_macro

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

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

#if 1 || DT_NODE_EXISTS(DT_DRV_INST(0))

enum mode {
    ZMK_BHV_SIMPLE_MACRO_MODE_KEY_UP = 0,
    ZMK_BHV_SIMPLE_MACRO_MODE_KEY_DOWN = 1,
    ZMK_BHV_SIMPLE_MACRO_MODE_HOLD = 2,
};

struct behavior_simple_macro_config {
    int mode;
    int sleep;
    int behavior_count;
    struct zmk_behavior_binding *behaviors;
};

struct behavior_simple_macro_data {};

static int behavior_simple_macro_init(struct device *dev) { return 0; }

static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t _, u32_t __) {
    const struct behavior_simple_macro_config *cfg = dev->config_info;

    if (cfg->mode == ZMK_BHV_SIMPLE_MACRO_MODE_KEY_DOWN) {
        LOG_DBG("tapping on key down");
        for (int index = 0; index < cfg->behavior_count; index++) {
            struct device *behavior = device_get_binding(cfg->behaviors[index].behavior_dev);
            if (behavior) {
                LOG_DBG("tapping: binding name: %s",
                        log_strdup(cfg->behaviors[index].behavior_dev));
                k_msleep(cfg->sleep);
                behavior_keymap_binding_pressed(behavior, position, cfg->behaviors[index].param1,
                                                cfg->behaviors[index].param2);
                k_msleep(cfg->sleep);
                behavior_keymap_binding_released(behavior, position, cfg->behaviors[index].param1,
                                                 cfg->behaviors[index].param2);
            }
        }
    } else if (cfg->mode == ZMK_BHV_SIMPLE_MACRO_MODE_HOLD) {
        for (int index = 0; index < cfg->behavior_count; index++) {
            struct device *behavior = device_get_binding(cfg->behaviors[index].behavior_dev);
            if (behavior) {
                LOG_DBG("pressing: binding name: %s",
                        log_strdup(cfg->behaviors[index].behavior_dev));
                behavior_keymap_binding_pressed(behavior, position, cfg->behaviors[index].param1,
                                                cfg->behaviors[index].param2);
            }
        }
    }

    return 0;
}

static int on_keymap_binding_released(struct device *dev, u32_t position, u32_t _, u32_t __) {
    const struct behavior_simple_macro_config *cfg = dev->config_info;

    if (cfg->mode == ZMK_BHV_SIMPLE_MACRO_MODE_KEY_UP) {
        LOG_DBG("tapping on key up");
        for (int index = 0; index < cfg->behavior_count; index++) {
            struct device *behavior = device_get_binding(cfg->behaviors[index].behavior_dev);
            if (behavior) {
                LOG_DBG("tapping: binding name: %s",
                        log_strdup(cfg->behaviors[index].behavior_dev));
                k_msleep(cfg->sleep);
                behavior_keymap_binding_pressed(behavior, position, cfg->behaviors[index].param1,
                                                cfg->behaviors[index].param2);
                k_msleep(cfg->sleep);
                behavior_keymap_binding_released(behavior, position, cfg->behaviors[index].param1,
                                                 cfg->behaviors[index].param2);
            }
        }
    } else if (cfg->mode == ZMK_BHV_SIMPLE_MACRO_MODE_HOLD) {
        for (int index = cfg->behavior_count - 1; index >= 0; index--) {
            struct device *behavior = device_get_binding(cfg->behaviors[index].behavior_dev);
            if (behavior) {
                LOG_DBG("releasing: binding name: %s",
                        log_strdup(cfg->behaviors[index].behavior_dev));
                behavior_keymap_binding_released(behavior, position, cfg->behaviors[index].param1,
                                                 cfg->behaviors[index].param2);
            }
        }
    }

    return 0;
}

static const struct behavior_driver_api behavior_simple_macro_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define KP_INST(n)                                                                                 \
    static struct zmk_behavior_binding                                                             \
        behavior_simple_macro_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =               \
            TRANSFORMED_BINDINGS(n);                                                               \
    static struct behavior_simple_macro_config behavior_simple_macro_config_##n = {                \
        .behaviors = behavior_simple_macro_config_##n##_bindings,                                  \
        .behavior_count = DT_INST_PROP_LEN(n, bindings),                                           \
        .sleep = DT_INST_PROP(n, sleep),                                                           \
        .mode = DT_ENUM_IDX(DT_DRV_INST(n), mode),                                                 \
    };                                                                                             \
    static struct behavior_simple_macro_data behavior_simple_macro_data_##n;                       \
    DEVICE_AND_API_INIT(behavior_simple_macro_##n, DT_INST_LABEL(n), behavior_simple_macro_init,   \
                        &behavior_simple_macro_data_##n, &behavior_simple_macro_config_##n,        \
                        APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                          \
                        &behavior_simple_macro_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)
#endif