/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum behavior_macro_mode {
    MACRO_MODE_TAP,
    MACRO_MODE_PRESS,
    MACRO_MODE_RELEASE,
};

enum param_source { PARAM_SOURCE_BINDING, PARAM_SOURCE_MACRO_1ST, PARAM_SOURCE_MACRO_2ND };

struct behavior_macro_trigger_state {
    uint32_t wait_ms;
    uint32_t tap_ms;
    enum behavior_macro_mode mode;
    uint16_t start_index;
    uint16_t count;
    enum param_source param1_source;
    enum param_source param2_source;
};

struct behavior_macro_state {
    struct behavior_macro_trigger_state release_state;

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    struct behavior_parameter_metadata_set set;
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

    uint32_t press_bindings_count;
};

struct behavior_macro_config {
    uint32_t default_wait_ms;
    uint32_t default_tap_ms;
    uint32_t count;
    struct zmk_behavior_binding bindings[];
};

#define TAP_MODE DEVICE_DT_NAME(DT_INST(0, zmk_macro_control_mode_tap))
#define PRESS_MODE DEVICE_DT_NAME(DT_INST(0, zmk_macro_control_mode_press))
#define REL_MODE DEVICE_DT_NAME(DT_INST(0, zmk_macro_control_mode_release))

#define TAP_TIME DEVICE_DT_NAME(DT_INST(0, zmk_macro_control_tap_time))
#define WAIT_TIME DEVICE_DT_NAME(DT_INST(0, zmk_macro_control_wait_time))
#define WAIT_REL DEVICE_DT_NAME(DT_INST(0, zmk_macro_pause_for_release))

#define P1TO1 DEVICE_DT_NAME(DT_INST(0, zmk_macro_param_1to1))
#define P1TO2 DEVICE_DT_NAME(DT_INST(0, zmk_macro_param_1to2))
#define P2TO1 DEVICE_DT_NAME(DT_INST(0, zmk_macro_param_2to1))
#define P2TO2 DEVICE_DT_NAME(DT_INST(0, zmk_macro_param_2to2))

#define ZM_IS_NODE_MATCH(a, b) (strcmp(a, b) == 0)
#define IS_TAP_MODE(dev) ZM_IS_NODE_MATCH(dev, TAP_MODE)
#define IS_PRESS_MODE(dev) ZM_IS_NODE_MATCH(dev, PRESS_MODE)
#define IS_RELEASE_MODE(dev) ZM_IS_NODE_MATCH(dev, REL_MODE)

#define IS_TAP_TIME(dev) ZM_IS_NODE_MATCH(dev, TAP_TIME)
#define IS_WAIT_TIME(dev) ZM_IS_NODE_MATCH(dev, WAIT_TIME)
#define IS_PAUSE(dev) ZM_IS_NODE_MATCH(dev, WAIT_REL)

#define IS_P1TO1(dev) ZM_IS_NODE_MATCH(dev, P1TO1)
#define IS_P1TO2(dev) ZM_IS_NODE_MATCH(dev, P1TO2)
#define IS_P2TO1(dev) ZM_IS_NODE_MATCH(dev, P2TO1)
#define IS_P2TO2(dev) ZM_IS_NODE_MATCH(dev, P2TO2)

static bool handle_control_binding(struct behavior_macro_trigger_state *state,
                                   const struct zmk_behavior_binding *binding) {
    if (IS_TAP_MODE(binding->behavior_dev)) {
        state->mode = MACRO_MODE_TAP;
        LOG_DBG("macro mode set: tap");
    } else if (IS_PRESS_MODE(binding->behavior_dev)) {
        state->mode = MACRO_MODE_PRESS;
        LOG_DBG("macro mode set: press");
    } else if (IS_RELEASE_MODE(binding->behavior_dev)) {
        state->mode = MACRO_MODE_RELEASE;
        LOG_DBG("macro mode set: release");
    } else if (IS_TAP_TIME(binding->behavior_dev)) {
        state->tap_ms = binding->param1;
        LOG_DBG("macro tap time set: %d", state->tap_ms);
    } else if (IS_WAIT_TIME(binding->behavior_dev)) {
        state->wait_ms = binding->param1;
        LOG_DBG("macro wait time set: %d", state->wait_ms);
    } else if (IS_P1TO1(binding->behavior_dev)) {
        state->param1_source = PARAM_SOURCE_MACRO_1ST;
        LOG_DBG("macro param: 1to1");
    } else if (IS_P1TO2(binding->behavior_dev)) {
        state->param2_source = PARAM_SOURCE_MACRO_1ST;
        LOG_DBG("macro param: 1to2");
    } else if (IS_P2TO1(binding->behavior_dev)) {
        state->param1_source = PARAM_SOURCE_MACRO_2ND;
        LOG_DBG("macro param: 2to1");
    } else if (IS_P2TO2(binding->behavior_dev)) {
        state->param2_source = PARAM_SOURCE_MACRO_2ND;
        LOG_DBG("macro param: 2to2");
    } else {
        return false;
    }

    return true;
}

static int behavior_macro_init(const struct device *dev) {
    const struct behavior_macro_config *cfg = dev->config;
    struct behavior_macro_state *state = dev->data;
    state->press_bindings_count = cfg->count;
    state->release_state.start_index = cfg->count;
    state->release_state.count = 0;

    LOG_DBG("Precalculate initial release state:");
    for (int i = 0; i < cfg->count; i++) {
        if (handle_control_binding(&state->release_state, &cfg->bindings[i])) {
            // Updated state used for initial state on release.
        } else if (IS_PAUSE(cfg->bindings[i].behavior_dev)) {
            state->release_state.start_index = i + 1;
            state->release_state.count = cfg->count - state->release_state.start_index;
            state->press_bindings_count = i;
            LOG_DBG("Release will resume at %d", state->release_state.start_index);
            break;
        } else {
            // Ignore regular invokable bindings
        }
    }

    return 0;
};

static uint32_t select_param(enum param_source param_source, uint32_t source_binding,
                             const struct zmk_behavior_binding *macro_binding) {
    switch (param_source) {
    case PARAM_SOURCE_MACRO_1ST:
        return macro_binding->param1;
    case PARAM_SOURCE_MACRO_2ND:
        return macro_binding->param2;
    default:
        return source_binding;
    }
};

static void replace_params(struct behavior_macro_trigger_state *state,
                           struct zmk_behavior_binding *binding,
                           const struct zmk_behavior_binding *macro_binding) {
    binding->param1 = select_param(state->param1_source, binding->param1, macro_binding);
    binding->param2 = select_param(state->param2_source, binding->param2, macro_binding);

    state->param1_source = PARAM_SOURCE_BINDING;
    state->param2_source = PARAM_SOURCE_BINDING;
}

static void queue_macro(struct zmk_behavior_binding_event *event,
                        const struct zmk_behavior_binding bindings[],
                        struct behavior_macro_trigger_state state,
                        const struct zmk_behavior_binding *macro_binding) {
    LOG_DBG("Iterating macro bindings - starting: %d, count: %d", state.start_index, state.count);
    for (int i = state.start_index; i < state.start_index + state.count; i++) {
        if (!handle_control_binding(&state, &bindings[i])) {
            struct zmk_behavior_binding binding = bindings[i];
            replace_params(&state, &binding, macro_binding);

            switch (state.mode) {
            case MACRO_MODE_TAP:
                zmk_behavior_queue_add(event, binding, true, state.tap_ms);
                zmk_behavior_queue_add(event, binding, false, state.wait_ms);
                break;
            case MACRO_MODE_PRESS:
                zmk_behavior_queue_add(event, binding, true, state.wait_ms);
                break;
            case MACRO_MODE_RELEASE:
                zmk_behavior_queue_add(event, binding, false, state.wait_ms);
                break;
            default:
                LOG_ERR("Unknown macro mode: %d", state.mode);
                break;
            }
        }
    }
}

static int on_macro_binding_pressed(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_macro_config *cfg = dev->config;
    struct behavior_macro_state *state = dev->data;
    struct behavior_macro_trigger_state trigger_state = {.mode = MACRO_MODE_TAP,
                                                         .tap_ms = cfg->default_tap_ms,
                                                         .wait_ms = cfg->default_wait_ms,
                                                         .start_index = 0,
                                                         .count = state->press_bindings_count};

    queue_macro(&event, cfg->bindings, trigger_state, binding);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_macro_binding_released(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_macro_config *cfg = dev->config;
    struct behavior_macro_state *state = dev->data;

    queue_macro(&event, cfg->bindings, state->release_state, binding);

    return ZMK_BEHAVIOR_OPAQUE;
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static void assign_values_to_set(enum param_source param_source,
                                 struct behavior_parameter_metadata_set *set,
                                 const struct behavior_parameter_value_metadata *values,
                                 size_t values_len) {
    if (param_source == PARAM_SOURCE_MACRO_1ST) {
        set->param1_values = values;
        set->param1_values_len = values_len;
    } else {
        set->param2_values = values;
        set->param2_values_len = values_len;
    }
}

// This function will dynamically determine the parameter metadata for a particular macro by
// inspecting the macro *bindings* to see what behaviors in that list receive the macro parameters,
// and then using the metadata from those behaviors for the macro itself.
//
// Care need be taken, where a behavior in the list takes two parameters, and the macro passes along
// a value for the *second* parameter, we need to make sure we find the right metadata set for the
// referenced behavior that matches the first parameter.
static int get_macro_parameter_metadata(const struct device *macro,
                                        struct behavior_parameter_metadata *param_metadata) {
    const struct behavior_macro_config *cfg = macro->config;
    struct behavior_macro_state *data = macro->data;
    struct behavior_macro_trigger_state state = {0};

    for (int i = 0; (i < cfg->count) && (!data->set.param1_values || !data->set.param2_values);
         i++) {
        if (handle_control_binding(&state, &cfg->bindings[i]) ||
            (state.param1_source == PARAM_SOURCE_BINDING &&
             state.param2_source == PARAM_SOURCE_BINDING)) {
            continue;
        }

        LOG_DBG("checking %d for the given state", i);

        struct behavior_parameter_metadata binding_meta;
        int err = behavior_get_parameter_metadata(
            zmk_behavior_get_binding(cfg->bindings[i].behavior_dev), &binding_meta);
        if (err < 0 || binding_meta.sets_len == 0) {
            LOG_WRN("Failed to fetch macro binding parameter details %d", err);
            return -ENOTSUP;
        }

        // If both macro parameters get passed to this one entry, use
        // the metadata for this behavior verbatim.
        if (state.param1_source != PARAM_SOURCE_BINDING &&
            state.param2_source != PARAM_SOURCE_BINDING) {
            param_metadata->sets_len = binding_meta.sets_len;
            param_metadata->sets = binding_meta.sets;
            return 0;
        }

        if (state.param1_source != PARAM_SOURCE_BINDING) {
            assign_values_to_set(state.param1_source, &data->set,
                                 binding_meta.sets[0].param1_values,
                                 binding_meta.sets[0].param1_values_len);
        }

        if (state.param2_source != PARAM_SOURCE_BINDING) {
            // For the param2 metadata, we need to find a set that matches fully bound first
            // parameter of our macro entry, and use the metadata from that set.
            for (int s = 0; s < binding_meta.sets_len; s++) {
                if (zmk_behavior_validate_param_values(binding_meta.sets[s].param1_values,
                                                       binding_meta.sets[s].param1_values_len,
                                                       cfg->bindings[i].param1) >= 0) {
                    assign_values_to_set(state.param2_source, &data->set,
                                         binding_meta.sets[s].param2_values,
                                         binding_meta.sets[s].param2_values_len);
                    break;
                }
            }
        }

        state.param1_source = PARAM_SOURCE_BINDING;
        state.param2_source = PARAM_SOURCE_BINDING;
    }

    param_metadata->sets_len = 1;
    param_metadata->sets = &data->set;

    return 0;
}

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_driver_api behavior_macro_driver_api = {
    .binding_pressed = on_macro_binding_pressed,
    .binding_released = on_macro_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = get_macro_parameter_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define TRANSFORMED_BEHAVIORS(n)                                                                   \
    {LISTIFY(DT_PROP_LEN(n, bindings), ZMK_KEYMAP_EXTRACT_BINDING, (, ), n)},

#define MACRO_INST(inst)                                                                           \
    static struct behavior_macro_state behavior_macro_state_##inst = {};                           \
    static struct behavior_macro_config behavior_macro_config_##inst = {                           \
        .default_wait_ms = DT_PROP_OR(inst, wait_ms, CONFIG_ZMK_MACRO_DEFAULT_WAIT_MS),            \
        .default_tap_ms = DT_PROP_OR(inst, tap_ms, CONFIG_ZMK_MACRO_DEFAULT_TAP_MS),               \
        .count = DT_PROP_LEN(inst, bindings),                                                      \
        .bindings = TRANSFORMED_BEHAVIORS(inst)};                                                  \
    BEHAVIOR_DT_DEFINE(inst, behavior_macro_init, NULL, &behavior_macro_state_##inst,              \
                       &behavior_macro_config_##inst, POST_KERNEL,                                 \
                       CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_macro_driver_api);

DT_FOREACH_STATUS_OKAY(zmk_behavior_macro, MACRO_INST)
DT_FOREACH_STATUS_OKAY(zmk_behavior_macro_one_param, MACRO_INST)
DT_FOREACH_STATUS_OKAY(zmk_behavior_macro_two_param, MACRO_INST)
