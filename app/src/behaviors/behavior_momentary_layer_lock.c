/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_momentary_layer_lock

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/momentary_layer.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_mo_lock_config {
    struct zmk_behavior_binding fallback_binding;
};

struct behavior_mo_lock_data {
    bool is_fallback_binding_pressed;
};

static int behavior_mo_lock_init(const struct device *dev) { return 0; };

static int mo_lock_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    LOG_DBG("%d molock pressed", event.position);

    zmk_keymap_layers_state_t locked_layers = zmk_lock_active_momentary_layers();

    if (!locked_layers) {
        LOG_DBG("no layers locked, falling back to %s", binding->behavior_dev);

        const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
        const struct behavior_mo_lock_config *config = dev->config;
        struct zmk_behavior_binding fallback_binding = config->fallback_binding;
        struct behavior_mo_lock_data *data = dev->data;

        data->is_fallback_binding_pressed = true;
        return behavior_keymap_binding_pressed(&fallback_binding, event);
    } else {
        LOG_DBG("locked layers: %#08x", locked_layers);
    }

    return 0;
}

static int mo_lock_keymap_binding_released(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    LOG_DBG("%d molock released", event.position);

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_mo_lock_data *data = dev->data;

    if (data->is_fallback_binding_pressed) {
        const struct behavior_mo_lock_config *config = dev->config;
        struct zmk_behavior_binding fallback_binding = config->fallback_binding;

        data->is_fallback_binding_pressed = false;
        return behavior_keymap_binding_released(&fallback_binding, event);
    }

    return 0;
}

#define KP_INST(n)                                                                                 \
    static struct behavior_mo_lock_config behavior_mo_lock_config_##n = {                          \
        .fallback_binding =                                                                        \
            {                                                                                      \
                .behavior_dev = DT_PROP(DT_INST_PHANDLE_BY_IDX(n, bindings, 0), label),            \
                .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(n, bindings, 0, param1), (0),    \
                                      (DT_INST_PHA_BY_IDX(n, bindings, 0, param1))),               \
                .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(n, bindings, 0, param2), (0),    \
                                      (DT_INST_PHA_BY_IDX(n, bindings, 0, param))),                \
            },                                                                                     \
    };                                                                                             \
    static struct behavior_mo_lock_data behavior_mo_lock_data_##n;                                 \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, behavior_mo_lock_init, NULL, &behavior_mo_lock_data_##n,              \
                          &behavior_mo_lock_config_##n, APPLICATION,                               \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_mo_lock_driver_api);

static const struct behavior_driver_api behavior_mo_lock_driver_api = {
    .binding_pressed = mo_lock_keymap_binding_pressed,
    .binding_released = mo_lock_keymap_binding_released,
};

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */