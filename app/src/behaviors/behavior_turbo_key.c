/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_turbo_key

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_turbo_config {
    int tap_ms;
    int wait_ms;
    int toggle_term_ms;
    const struct zmk_behavior_binding binding;
};

#define ZMK_BHV_TURBO_MAX_ACTIVE 5

struct active_turbo {
    const struct behavior_turbo_config *config;
    uint32_t position;
    bool is_active;
    bool is_pressed;

    int32_t press_time;

    // Timer Data
    bool timer_started;
    bool timer_cancelled;
    bool turbo_decided;
    int64_t release_at;
    struct k_work_delayable release_timer;
};

struct active_turbo active_turbos[ZMK_BHV_TURBO_MAX_ACTIVE] = {};

static struct active_turbo *find_active_turbo(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_TURBO_MAX_ACTIVE; i++) {
        if (active_turbos[i].is_active) {
            return &active_turbos[i];
        }
    }
    return NULL;
}

static int new_turbo(uint32_t position, const struct behavior_turbo_config *config,
                     struct active_turbo **turbo) {
    for (int i = 0; i < ZMK_BHV_TURBO_MAX_ACTIVE; i++) {
        struct active_turbo *const ref_turbo = &active_turbos[i];
        if (!ref_turbo->is_active) {
            ref_turbo->is_active = true;
            ref_turbo->position = position;
            ref_turbo->config = config;
            ref_turbo->is_pressed = true;
            ref_turbo->press_time = k_uptime_get();
            ref_turbo->release_at = 0;
            ref_turbo->timer_started = true;
            ref_turbo->timer_cancelled = false;
            *turbo = ref_turbo;
            return 0;
        }
    }
    return -ENOMEM;
};

static int stop_timer(struct active_turbo *turbo) {
    int timer_cancel_result = k_work_cancel_delayable(&turbo->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        turbo->timer_cancelled = true;
    }
    return timer_cancel_result;
}

static void clear_turbo(struct active_turbo *turbo) {
    LOG_DBG("Turbo deactivated");
    turbo->is_active = false;
    stop_timer(turbo);
}

static void reset_timer(struct active_turbo *turbo, struct zmk_behavior_binding_event event) {
    turbo->release_at = event.timestamp + turbo->config->wait_ms;
    int32_t ms_left = turbo->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_work_schedule(&turbo->release_timer, K_MSEC(ms_left));
        LOG_DBG("Successfully reset turbo timer at position %d", turbo->position);
    }
}

static void behavior_turbo_timer_handler(struct k_work *item) {
    struct active_turbo *turbo = CONTAINER_OF(item, struct active_turbo, release_timer);
    if (!turbo->is_active) {
        return;
    }
    if (turbo->timer_cancelled) {
        return;
    }
    LOG_DBG("Turbo timer reached.");
    struct zmk_behavior_binding_event event = {.position = turbo->position,
                                               .timestamp = k_uptime_get()};
    zmk_behavior_queue_add(event.position, turbo->config->binding, true, turbo->config->tap_ms);
    zmk_behavior_queue_add(event.position, turbo->config->binding, false, 0);
    reset_timer(turbo, event);
}

static int behavior_turbo_key_init(const struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_TURBO_MAX_ACTIVE; i++) {
            k_work_init_delayable(&active_turbos[i].release_timer, behavior_turbo_timer_handler);
            clear_turbo(&active_turbos[i]);
        }
    }
    init_first_run = false;
    return 0;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_turbo_config *cfg = dev->config;

    struct active_turbo *turbo;
    turbo = find_active_turbo(event.position);
    if (turbo == NULL) {
        if (new_turbo(event.position, cfg, &turbo) == -ENOMEM) {
            LOG_ERR("Unable to create new turbo. Insufficient space in active_turbos[].");
            return ZMK_BEHAVIOR_OPAQUE;
        }
        LOG_DBG("%d created new turbo", event.position);
        zmk_behavior_queue_add(event.position, turbo->config->binding, true, turbo->config->tap_ms);
        zmk_behavior_queue_add(event.position, turbo->config->binding, false, 0);
        reset_timer(turbo, event);
    } else {
        clear_turbo(turbo);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_turbo_config *cfg = dev->config;

    struct active_turbo *turbo;
    turbo = find_active_turbo(event.position);
    if (turbo != NULL) {
        turbo->is_pressed = false;
        int32_t elapsedTime = k_uptime_get() - turbo->press_time;
        LOG_DBG("turbo elapsed time: %d", elapsedTime);
        if (elapsedTime > cfg->toggle_term_ms) {
            clear_turbo(turbo);
        }
    }
    return 0;
}

static const struct behavior_driver_api behavior_turbo_key_driver_api = {
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
    }

#define TURBO_INST(n)                                                                              \
    static struct behavior_turbo_config behavior_turbo_config_##n = {                              \
        .tap_ms = DT_INST_PROP(n, tap_ms),                                                         \
        .wait_ms = DT_INST_PROP(n, wait_ms),                                                       \
        .toggle_term_ms = DT_INST_PROP(n, toggle_term_ms),                                         \
        .binding = _TRANSFORM_ENTRY(0, n)};                                                        \
    DEVICE_DT_INST_DEFINE(n, behavior_turbo_key_init, NULL, NULL, &behavior_turbo_config_##n,      \
                          APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                          &behavior_turbo_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TURBO_INST)
