/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_behavior_mod_morph) ||                                           \
    DT_HAS_COMPAT_STATUS_OKAY(zmk_behavior_mod_morph_param)

struct behavior_mod_morph_config {
    struct zmk_behavior_binding normal_binding;
    struct zmk_behavior_binding morph_binding;
    zmk_mod_flags_t mods;
    zmk_mod_flags_t masked_mods;
    uint8_t binding_params;
};

struct behavior_mod_morph_data {
    struct zmk_behavior_binding *pressed_binding;
};

static int on_mod_morph_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_mod_morph_config *cfg = dev->config;
    struct behavior_mod_morph_data *data = dev->data;
    uint8_t map = cfg->binding_params;

    if (data->pressed_binding != NULL) {
        LOG_ERR("Can't press the same mod-morph twice");
        return -ENOTSUP;
    }

    if (zmk_hid_get_explicit_mods() & cfg->mods) {
        zmk_hid_masked_modifiers_set(cfg->masked_mods);
        data->pressed_binding = (struct zmk_behavior_binding *)&cfg->morph_binding;
    } else {
        data->pressed_binding = (struct zmk_behavior_binding *)&cfg->normal_binding;
        map = map >> 4;
    }
    if (map & 0b1100)
        data->pressed_binding->param1 = (map & 0b0100) ? binding->param1 : binding->param2;
    if (map & 0b0011)
        data->pressed_binding->param2 = (map & 0b0001) ? binding->param1 : binding->param2;
    return zmk_behavior_invoke_binding(data->pressed_binding, event, true);
}

static int on_mod_morph_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_mod_morph_data *data = dev->data;

    if (data->pressed_binding == NULL) {
        LOG_ERR("Mod-morph already released");
        return -ENOTSUP;
    }

    struct zmk_behavior_binding *pressed_binding = data->pressed_binding;
    data->pressed_binding = NULL;
    int err;
    err = zmk_behavior_invoke_binding(pressed_binding, event, false);
    zmk_hid_masked_modifiers_clear();
    return err;
}

static const struct behavior_driver_api behavior_mod_morph_driver_api = {
    .binding_pressed = on_mod_morph_binding_pressed,
    .binding_released = on_mod_morph_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

static int behavior_mod_morph_init(const struct device *dev) { return 0; }

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(node, bindings, idx)),                    \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),            \
                              (DT_PHA_BY_IDX(node, bindings, idx, param1))),                       \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),            \
                              (DT_PHA_BY_IDX(node, bindings, idx, param2))),                       \
    }

#define KP_INST(inst)                                                                              \
    BUILD_ASSERT((COND_CODE_0(DT_NODE_HAS_PROP(inst, binding_params), (0),                         \
                              ((((DT_PROP_BY_IDX(inst, binding_params, 0) >> 2) &                  \
                                 (DT_PROP_BY_IDX(inst, binding_params, 0))) |                      \
                                ((DT_PROP_BY_IDX(inst, binding_params, 1) >> 2) &                  \
                                 (DT_PROP_BY_IDX(inst, binding_params, 1)))))) == 0),              \
                 "Invalid binding parameters");                                                    \
    static struct behavior_mod_morph_config behavior_mod_morph_config_##inst = {                   \
        .normal_binding = _TRANSFORM_ENTRY(0, inst),                                               \
        .morph_binding = _TRANSFORM_ENTRY(1, inst),                                                \
        .mods = DT_PROP(inst, mods),                                                               \
        .masked_mods = COND_CODE_0(DT_NODE_HAS_PROP(inst, keep_mods), (DT_PROP(inst, mods)),       \
                                   (DT_PROP(inst, mods) & ~DT_PROP(inst, keep_mods))),             \
        .binding_params = (COND_CODE_0(DT_NODE_HAS_PROP(inst, binding_params), (0),                \
                                       ((DT_PROP_BY_IDX(inst, binding_params, 0) << 4) |           \
                                        (DT_PROP_BY_IDX(inst, binding_params, 1))))),              \
    };                                                                                             \
    static struct behavior_mod_morph_data behavior_mod_morph_data_##inst = {};                     \
    BEHAVIOR_DT_DEFINE(inst, behavior_mod_morph_init, NULL, &behavior_mod_morph_data_##inst,       \
                       &behavior_mod_morph_config_##inst, POST_KERNEL,                             \
                       CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_mod_morph_driver_api);

DT_FOREACH_STATUS_OKAY(zmk_behavior_mod_morph, KP_INST)
DT_FOREACH_STATUS_OKAY(zmk_behavior_mod_morph_param, KP_INST)

#endif
