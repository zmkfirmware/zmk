/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_tri_state

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define ZMK_BHV_MAX_ACTIVE_TRI_STATES 10

struct behavior_tri_state_config {
    int32_t ignored_key_positions_len;
    int32_t ignored_layers_len;
    struct zmk_behavior_binding start_behavior;
    struct zmk_behavior_binding continue_behavior;
    struct zmk_behavior_binding end_behavior;
    int32_t ignored_layers;
    int32_t timeout_ms;
    int tap_ms;
    uint8_t ignored_key_positions[];
};

struct active_tri_state {
    bool is_active;
    bool is_pressed;
    bool first_press;
    uint32_t position;
    const struct behavior_tri_state_config *config;
    struct k_work_delayable release_timer;
    int64_t release_at;
    bool timer_started;
    bool timer_cancelled;
};

static int stop_timer(struct active_tri_state *tri_state) {
    int timer_cancel_result = k_work_cancel_delayable(&tri_state->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        tri_state->timer_cancelled = true;
    }
    return timer_cancel_result;
}

static void reset_timer(int32_t timestamp, struct active_tri_state *tri_state) {
    tri_state->release_at = timestamp + tri_state->config->timeout_ms;
    int32_t ms_left = tri_state->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_work_schedule(&tri_state->release_timer, K_MSEC(ms_left));
        LOG_DBG("Successfully reset tri-state timer");
    }
}

void trigger_end_behavior(struct active_tri_state *si) {
    zmk_behavior_queue_add(si->position, si->config->end_behavior, true, si->config->tap_ms);
    zmk_behavior_queue_add(si->position, si->config->end_behavior, false, 0);
}

void behavior_tri_state_timer_handler(struct k_work *item) {
    struct active_tri_state *tri_state = CONTAINER_OF(item, struct active_tri_state, release_timer);
    if (!tri_state->is_active || tri_state->timer_cancelled || tri_state->is_pressed) {
        return;
    }
    LOG_DBG("Tri-state deactivated due to timer");
    tri_state->is_active = false;
    trigger_end_behavior(tri_state);
}

static void clear_tri_state(struct active_tri_state *tri_state) { tri_state->is_active = false; }

struct active_tri_state active_tri_states[ZMK_BHV_MAX_ACTIVE_TRI_STATES] = {};

static struct active_tri_state *find_tri_state(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_TRI_STATES; i++) {
        if (active_tri_states[i].position == position && active_tri_states[i].is_active) {
            return &active_tri_states[i];
        }
    }
    return NULL;
}

static int new_tri_state(uint32_t position, const struct behavior_tri_state_config *config,
                         struct active_tri_state **tri_state) {
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_TRI_STATES; i++) {
        struct active_tri_state *const ref_tri_state = &active_tri_states[i];
        if (!ref_tri_state->is_active) {
            ref_tri_state->position = position;
            ref_tri_state->config = config;
            ref_tri_state->is_active = true;
            ref_tri_state->is_pressed = false;
            ref_tri_state->first_press = true;
            *tri_state = ref_tri_state;
            return 0;
        }
    }
    return -ENOMEM;
}

static bool is_other_key_ignored(struct active_tri_state *tri_state, int32_t position) {
    for (int i = 0; i < tri_state->config->ignored_key_positions_len; i++) {
        if (tri_state->config->ignored_key_positions[i] == position) {
            return true;
        }
    }
    return false;
}

static bool is_layer_ignored(struct active_tri_state *tri_state, int32_t layer) {
    if ((BIT(layer) & tri_state->config->ignored_layers) != 0U) {
        return true;
    }
    return false;
}

static int on_tri_state_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_tri_state_config *cfg = dev->config;
    struct active_tri_state *tri_state;
    tri_state = find_tri_state(event.position);
    if (tri_state == NULL) {
        if (new_tri_state(event.position, cfg, &tri_state) == -ENOMEM) {
            LOG_ERR("Unable to create new tri_state. Insufficient space in "
                    "active_tri_states[].");
            return ZMK_BEHAVIOR_OPAQUE;
        }
        LOG_DBG("%d created new tri_state", event.position);
    }
    LOG_DBG("%d tri_state pressed", event.position);
    tri_state->is_pressed = true;
    if (tri_state->first_press) {
        behavior_keymap_binding_pressed((struct zmk_behavior_binding *)&cfg->start_behavior, event);
        behavior_keymap_binding_released((struct zmk_behavior_binding *)&cfg->start_behavior,
                                         event);
        tri_state->first_press = false;
    }
    behavior_keymap_binding_pressed((struct zmk_behavior_binding *)&cfg->continue_behavior, event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static void release_tri_state(struct zmk_behavior_binding_event event,
                              struct zmk_behavior_binding *continue_behavior) {
    struct active_tri_state *tri_state = find_tri_state(event.position);
    if (tri_state == NULL) {
        return;
    }
    tri_state->is_pressed = false;
    behavior_keymap_binding_released(continue_behavior, event);
    reset_timer(k_uptime_get(), tri_state);
}

static int on_tri_state_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_tri_state_config *cfg = dev->config;
    LOG_DBG("%d tri_state keybind released", event.position);
    release_tri_state(event, (struct zmk_behavior_binding *)&cfg->continue_behavior);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_tri_state_init(const struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_TRI_STATES; i++) {
            k_work_init_delayable(&active_tri_states[i].release_timer,
                                  behavior_tri_state_timer_handler);
            clear_tri_state(&active_tri_states[i]);
        }
    }
    init_first_run = false;
    return 0;
}

static const struct behavior_driver_api behavior_tri_state_driver_api = {
    .binding_pressed = on_tri_state_binding_pressed,
    .binding_released = on_tri_state_binding_released,
};

static int tri_state_listener(const zmk_event_t *eh);
static int tri_state_position_state_changed_listener(const zmk_event_t *eh);
static int tri_state_layer_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_tri_state, tri_state_listener);
ZMK_SUBSCRIPTION(behavior_tri_state, zmk_position_state_changed);
ZMK_SUBSCRIPTION(behavior_tri_state, zmk_layer_state_changed);

static int tri_state_listener(const zmk_event_t *eh) {
    if (as_zmk_position_state_changed(eh) != NULL) {
        return tri_state_position_state_changed_listener(eh);
    } else if (as_zmk_layer_state_changed(eh) != NULL) {
        return tri_state_layer_state_changed_listener(eh);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int tri_state_position_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_TRI_STATES; i++) {
        struct active_tri_state *tri_state = &active_tri_states[i];
        if (!tri_state->is_active) {
            continue;
        }
        if (tri_state->position == ev->position) {
            continue;
        }
        if (!is_other_key_ignored(tri_state, ev->position)) {
            LOG_DBG("Tri-State interrupted, ending at %d %d", tri_state->position, ev->position);
            tri_state->is_active = false;
            struct zmk_behavior_binding_event event = {.position = tri_state->position,
                                                       .timestamp = k_uptime_get()};
            if (tri_state->is_pressed) {
                behavior_keymap_binding_released(
                    (struct zmk_behavior_binding *)&tri_state->config->continue_behavior, event);
            }
            trigger_end_behavior(tri_state);
            return ZMK_EV_EVENT_BUBBLE;
        }
        if (ev->state) {
            stop_timer(tri_state);
        } else {
            reset_timer(ev->timestamp, tri_state);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int tri_state_layer_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    if (!ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_TRI_STATES; i++) {
        struct active_tri_state *tri_state = &active_tri_states[i];
        if (!tri_state->is_active) {
            continue;
        }
        if (!is_layer_ignored(tri_state, ev->layer)) {
            LOG_DBG("Tri-State layer changed, ending at %d %d", tri_state->position, ev->layer);
            tri_state->is_active = false;
            struct zmk_behavior_binding_event event = {.position = tri_state->position,
                                                       .timestamp = k_uptime_get()};
            if (tri_state->is_pressed) {
                behavior_keymap_binding_released(
                    (struct zmk_behavior_binding *)&tri_state->config->continue_behavior, event);
            }
            behavior_keymap_binding_pressed(
                (struct zmk_behavior_binding *)&tri_state->config->end_behavior, event);
            behavior_keymap_binding_released(
                (struct zmk_behavior_binding *)&tri_state->config->end_behavior, event);
            return ZMK_EV_EVENT_BUBBLE;
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    }

#define IF_BIT(n, prop, i) BIT(DT_PROP_BY_IDX(n, prop, i)) |

#define TRI_STATE_INST(n)                                                                          \
    static struct behavior_tri_state_config behavior_tri_state_config_##n = {                      \
        .ignored_key_positions = DT_INST_PROP(n, ignored_key_positions),                           \
        .ignored_key_positions_len = DT_INST_PROP_LEN(n, ignored_key_positions),                   \
        .ignored_layers = DT_INST_FOREACH_PROP_ELEM(n, ignored_layers, IF_BIT) 0,                  \
        .ignored_layers_len = DT_INST_PROP_LEN(n, ignored_layers),                                 \
        .timeout_ms = DT_INST_PROP(n, timeout_ms),                                                 \
        .tap_ms = DT_INST_PROP(n, tap_ms),                                                         \
        .start_behavior = _TRANSFORM_ENTRY(0, n),                                                  \
        .continue_behavior = _TRANSFORM_ENTRY(1, n),                                               \
        .end_behavior = _TRANSFORM_ENTRY(2, n)};                                                   \
    DEVICE_DT_INST_DEFINE(n, behavior_tri_state_init, NULL, NULL, &behavior_tri_state_config_##n,  \
                          APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                          &behavior_tri_state_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TRI_STATE_INST)
