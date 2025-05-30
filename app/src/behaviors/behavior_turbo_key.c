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

struct behavior_turbo_data {
    int32_t tap_ms;
    int32_t wait_ms;
    int32_t toggle_term_ms;

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

    uint32_t binding_count;
    struct zmk_behavior_binding binding;
    struct zmk_behavior_binding new_binding;
    const struct zmk_behavior_binding bindings[];
};

static int stop_timer(struct behavior_turbo_data *data) {
    int timer_cancel_result = k_work_cancel_delayable(&data->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        data->timer_cancelled = true;
    }
    return timer_cancel_result;
}

static void clear_turbo(struct behavior_turbo_data *data) {
    LOG_DBG("Turbo deactivated at position %d", data->position);
    data->is_active = false;
    stop_timer(data);
}

static void reset_timer(struct behavior_turbo_data *data, struct zmk_behavior_binding_event event) {
    data->release_at = event.timestamp + data->wait_ms;
    int32_t ms_left = data->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_work_schedule(&data->release_timer, K_MSEC(ms_left));
        LOG_DBG("Successfully reset turbo timer at position %d", data->position);
    }
}

static void press_turbo_binding(struct zmk_behavior_binding_event *event,
                                const struct behavior_turbo_data *data) {
    LOG_DBG("Pressing turbo binding %s, %d, %d", data->binding.behavior_dev, data->binding.param1,
            data->binding.param2);
    zmk_behavior_queue_add(event, data->binding, true, data->tap_ms);
    zmk_behavior_queue_add(event, data->binding, false, 0);
}

static void behavior_turbo_timer_handler(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);

    struct behavior_turbo_data *data =
        CONTAINER_OF(d_work, struct behavior_turbo_data, release_timer);

    if (!data->is_active || data->timer_cancelled) {
        return;
    }

    LOG_DBG("Turbo timer reached.");
    struct zmk_behavior_binding_event event = {.position = data->position,
                                               .timestamp = k_uptime_get()};

    press_turbo_binding(&event, data);
    reset_timer(data, event);
}

#define P1TO1 DEVICE_DT_NAME(DT_INST(0, zmk_turbo_param_1to1))
#define P1TO2 DEVICE_DT_NAME(DT_INST(0, zmk_turbo_param_1to2))
#define P2TO1 DEVICE_DT_NAME(DT_INST(0, zmk_turbo_param_2to1))
#define P2TO2 DEVICE_DT_NAME(DT_INST(0, zmk_turbo_param_2to2))

#define ZM_IS_NODE_MATCH(a, b) (strcmp(a, b) == 0)

#define IS_P1TO1(dev) ZM_IS_NODE_MATCH(dev, P1TO1)
#define IS_P1TO2(dev) ZM_IS_NODE_MATCH(dev, P1TO2)
#define IS_P2TO1(dev) ZM_IS_NODE_MATCH(dev, P2TO1)
#define IS_P2TO2(dev) ZM_IS_NODE_MATCH(dev, P2TO2)

static bool handle_control_binding(struct behavior_turbo_data *data,
                                   struct zmk_behavior_binding *binding,
                                   const struct zmk_behavior_binding new_binding) {
    if (IS_P1TO1(new_binding.behavior_dev)) {
        data->new_binding.param1 = binding->param1;
        LOG_DBG("turbo param: 1to1: %d", binding->param1);
    } else if (IS_P1TO2(new_binding.behavior_dev)) {
        data->new_binding.param2 = binding->param1;
        LOG_DBG("turbo param: 1to2");
    } else if (IS_P2TO1(new_binding.behavior_dev)) {
        data->new_binding.param1 = binding->param2;
        LOG_DBG("turbo param: 2to1");
    } else if (IS_P2TO2(new_binding.behavior_dev)) {
        data->new_binding.param2 = binding->param2;
        LOG_DBG("turbo param: 2to2");
    } else {
        return false;
    }

    return true;
}

static uint8_t get_binding_without_parameters_count(struct behavior_turbo_data *data) {
    uint8_t bindings_without_parameters = 0;

    for (int i = 0; i < data->binding_count; i++) {
        struct zmk_behavior_binding binding = data->bindings[i];
        if (!handle_control_binding(data, &binding, binding)) {
            bindings_without_parameters++;
        }
    }

    return bindings_without_parameters;
}

static void squash_params(struct behavior_turbo_data *data, struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding *new_bindings) {
    uint8_t new_bindings_index = 0;
    LOG_DBG("turbo bindings count is %d", data->binding_count);

    for (int i = 0; i < data->binding_count; i++) {
        bool is_control_binding = handle_control_binding(data, binding, data->bindings[i]);

        if (!is_control_binding) {
            data->new_binding.behavior_dev = data->bindings[i].behavior_dev;

            if (!data->new_binding.param1) {
                data->new_binding.param1 = data->bindings[i].param1;
            }

            if (!data->new_binding.param2) {
                data->new_binding.param2 = data->bindings[i].param1;
            }

            new_bindings[new_bindings_index] = data->new_binding;
            new_bindings_index++;
        }

        LOG_DBG("current turbo binding at index %d is %s, %d, %d", i,
                data->new_binding.behavior_dev, data->new_binding.param1, data->new_binding.param2);
    }
}

static int on_turbo_binding_pressed(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    struct behavior_turbo_data *data = dev->data;

    struct zmk_behavior_binding new_bindings[get_binding_without_parameters_count(data)];
    squash_params(data, binding, new_bindings);

    data->binding = new_bindings[0];

    if (!data->is_active) {
        data->is_active = true;

        LOG_DBG("Started new turbo at position %d", event.position);

        data->press_time = k_uptime_get();
        data->position = event.position;

        press_turbo_binding(&event, data);
        reset_timer(data, event);
    } else {
        clear_turbo(data);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_turbo_binding_released(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    struct behavior_turbo_data *data = dev->data;

    if (data->is_active) {
        data->is_pressed = false;
        int32_t elapsedTime = k_uptime_get() - data->press_time;
        LOG_DBG("turbo elapsed time: %d", elapsedTime);
        if (elapsedTime > data->toggle_term_ms) {
            clear_turbo(data);
        }
    }
    return 0;
}

static int behavior_turbo_key_init(const struct device *dev) {
    struct behavior_turbo_data *data = dev->data;
    k_work_init_delayable(&data->release_timer, behavior_turbo_timer_handler);
    return 0;
};

#define TRANSFORMED_BEHAVIORS(n)                                                                   \
    {LISTIFY(DT_PROP_LEN(n, bindings), ZMK_KEYMAP_EXTRACT_BINDING, (, ), n)}

static const struct behavior_driver_api behavior_turbo_key_driver_api = {
    .binding_pressed = on_turbo_binding_pressed,
    .binding_released = on_turbo_binding_released,
};

#define TURBO_INST(n)                                                                              \
    static struct behavior_turbo_data behavior_turbo_data_##n = {                                  \
        .tap_ms = DT_PROP(n, tap_ms),                                                              \
        .wait_ms = DT_PROP(n, wait_ms),                                                            \
        .toggle_term_ms = DT_PROP(n, toggle_term_ms),                                              \
        .bindings = TRANSFORMED_BEHAVIORS(n),                                                      \
        .binding_count = DT_PROP_LEN(n, bindings),                                                 \
        .is_active = false,                                                                        \
        .is_pressed = false};                                                                      \
    BEHAVIOR_DT_DEFINE(n, behavior_turbo_key_init, NULL, &behavior_turbo_data_##n, NULL,           \
                       POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                           \
                       &behavior_turbo_key_driver_api);

DT_FOREACH_STATUS_OKAY(zmk_behavior_turbo_key, TURBO_INST)
DT_FOREACH_STATUS_OKAY(zmk_behavior_turbo_key_one_param, TURBO_INST)
DT_FOREACH_STATUS_OKAY(zmk_behavior_turbo_key_two_param, TURBO_INST)
