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
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/dynamic-macros.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_behavior_dynamic_macro

#define HID_KEY_USAGE_PAGE 0x70000

int8_t total_recorded_actions = 0;

struct behavior_dynamic_macro_bind {
    uint32_t wait_ms;
    bool pressed;
    struct zmk_behavior_binding binding;
};

struct behavior_dynamic_macro_state {
    bool recording;
    uint32_t lastEventTime;
    uint32_t count;
    struct behavior_dynamic_macro_bind bindings[CONFIG_ZMK_DYNAMIC_MACRO_MAX_ACTIONS];
};

struct behavior_dynamic_macro_config {
    uint32_t wait_ms;
    bool no_output;
};

#define ZMK_BHV_RECORDING_MACRO_MAX 10

struct recording_macro {
    uint32_t count;
    uint32_t position;
    bool recording;
    const struct behavior_dynamic_macro_config *config;
    struct behavior_dynamic_macro_state *state;
};

struct recording_macro recording_macros[ZMK_BHV_RECORDING_MACRO_MAX] = {};

static struct recording_macro *find_recording_macro(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_RECORDING_MACRO_MAX; i++) {
        if (recording_macros[i].position == position && recording_macros[i].recording) {
            return &recording_macros[i];
        }
    }
    return NULL;
}

static struct recording_macro *find_macro_at_position(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_RECORDING_MACRO_MAX; i++) {
        if (recording_macros[i].position == position) {
            return &recording_macros[i];
        }
    }
    return NULL;
}

static int new_recording_macro(uint32_t position,
                               const struct behavior_dynamic_macro_config *config,
                               struct behavior_dynamic_macro_state *state,
                               struct recording_macro **macro) {
    for (int i = 0; i < ZMK_BHV_RECORDING_MACRO_MAX; i++) {
        struct recording_macro *const ref_macro = &recording_macros[i];
        if (!ref_macro->recording) {
            ref_macro->recording = true;
            ref_macro->count = 0;
            ref_macro->position = position;
            ref_macro->config = config;
            ref_macro->state = state;
            *macro = ref_macro;
            return 0;
        }
    }
    return -ENOMEM;
}

static void queue_dynamic_macro(uint32_t position, uint32_t time,
                                struct behavior_dynamic_macro_state *state) {
    LOG_DBG("Iterating dynamic macro bindings - count: %d", state->count);
    for (int i = 0; i < state->count; i++) {
        uint32_t wait_ms = time;
        if (time == -1) {
            wait_ms = state->bindings[i].wait_ms;
        }
        zmk_behavior_queue_add(position, state->bindings[i].binding, state->bindings[i].pressed,
                               wait_ms);
    }
}

static int on_dynamic_macro_binding_pressed(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_dynamic_macro_config *cfg = dev->config;
    struct behavior_dynamic_macro_state *state = dev->data;

    if (binding->param1 == PLAY) {
        if (state->recording) {
            LOG_ERR("Macro is currently recording, can't play");
        } else {
            LOG_DBG("Playing Dynamic Macro");
            queue_dynamic_macro(event.position, cfg->wait_ms, state);
        }
    } else if (binding->param1 == RECORD) {
        state->recording = !state->recording;
        LOG_DBG("Recording Status: %d", state->recording);
        if (state->recording) {
            struct recording_macro *macro;
            macro = find_recording_macro(event.position);
            if (new_recording_macro(event.position, cfg, state, &macro) == -ENOMEM) {
                LOG_ERR("Unable to record new macro. Insufficient space in recording_macros[]");
                return ZMK_BEHAVIOR_OPAQUE;
            }
            LOG_DBG("Recording new macro: %d", event.position);

            struct recording_macro *old_macro;
            old_macro = find_macro_at_position(event.position);
            if (old_macro) {
                total_recorded_actions -= old_macro->state->count;
            }
            macro->count = 0;
        } else {
            struct recording_macro *macro;
            macro = find_recording_macro(event.position);
            macro->recording = false;
            macro->state->count = macro->count;
        }
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_dynamic_macro_binding_released(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_dynamic_macro_init(const struct device *dev) { return 0; };

static const struct behavior_driver_api behavior_dynamic_macro_driver_api = {
    .binding_pressed = on_dynamic_macro_binding_pressed,
    .binding_released = on_dynamic_macro_binding_released,
};

static int dynamic_macro_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_dynamic_macro, dynamic_macro_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_dynamic_macro, zmk_keycode_state_changed);

static int dynamic_macro_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < ZMK_BHV_RECORDING_MACRO_MAX; i++) {
        struct recording_macro *macro = &recording_macros[i];
        if (macro->recording && total_recorded_actions < CONFIG_ZMK_DYNAMIC_MACRO_MAX_ACTIONS) {
            uint32_t eventTime = k_uptime_get();
            uint32_t elapsedTime = eventTime - macro->state->lastEventTime;
            macro->state->lastEventTime = eventTime;
            if (ev->state) {
                macro->state->bindings[macro->count].pressed = true;
            } else {
                macro->state->bindings[macro->count].pressed = false;
            }
            macro->state->bindings[macro->count].binding.behavior_dev = "KEY_PRESS";
            macro->state->bindings[macro->count].binding.param1 = HID_KEY_USAGE_PAGE + ev->keycode;
            macro->state->bindings[macro->count].binding.param2 = 0;

            if (macro->count > 0) {
                macro->state->bindings[macro->count - 1].wait_ms = elapsedTime;
            }

            macro->count++;
            total_recorded_actions++;

            if (macro->config->no_output) {
                return ZMK_EV_EVENT_HANDLED;
            }
            return ZMK_EV_EVENT_BUBBLE;
        } else if (total_recorded_actions >= CONFIG_ZMK_DYNAMIC_MACRO_MAX_ACTIONS) {
            LOG_ERR(
                "Action not recorded, not enough space, CONFIG_ZMK_DYNAMIC_MACRO_MAX_ACTIONS %d",
                CONFIG_ZMK_DYNAMIC_MACRO_MAX_ACTIONS);
            if (macro->config->no_output) {
                return ZMK_EV_EVENT_HANDLED;
            }
            return ZMK_EV_EVENT_BUBBLE;
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

#define DYNAMIC_MACRO_INST(n)                                                                      \
    static struct behavior_dynamic_macro_state behavior_dynamic_macro_state_##n = {                \
        .recording = false, .count = 0};                                                           \
    static struct behavior_dynamic_macro_config behavior_dynamic_macro_config_##n = {              \
        .wait_ms = DT_INST_PROP_OR(n, wait_ms, -1), .no_output = DT_INST_PROP(n, no_output)};      \
    DEVICE_DT_INST_DEFINE(n, behavior_dynamic_macro_init, NULL, &behavior_dynamic_macro_state_##n, \
                          &behavior_dynamic_macro_config_##n, APPLICATION,                         \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                     \
                          &behavior_dynamic_macro_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DYNAMIC_MACRO_INST)
