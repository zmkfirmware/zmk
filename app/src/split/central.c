/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <zephyr/init.h>

#include <zmk/stdlib.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/split/central.h>
#include <zmk/split/service.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

K_MSGQ_DEFINE(peripheral_event_msgq, sizeof(struct zmk_position_state_changed),
              CONFIG_ZMK_SPLIT_CENTRAL_POSITION_QUEUE_SIZE, 4);

void peripheral_event_work_callback(struct k_work *work) {
    struct zmk_position_state_changed ev;
    while (k_msgq_get(&peripheral_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger key position state change for %d", ev.position);
        raise_zmk_position_state_changed(ev);
    }
}

K_WORK_DEFINE(peripheral_event_work, peripheral_event_work_callback);

void zmk_position_state_change_handle(struct zmk_position_state_changed *ev) {
    k_msgq_put(&peripheral_event_msgq, ev, K_NO_WAIT);
    k_work_submit(&peripheral_event_work);
}

#if ZMK_KEYMAP_HAS_SENSORS
K_MSGQ_DEFINE(peripheral_sensor_event_msgq, sizeof(struct zmk_sensor_event),
              CONFIG_ZMK_SPLIT_CENTRAL_POSITION_QUEUE_SIZE, 4);

void peripheral_sensor_event_work_callback(struct k_work *work) {
    struct zmk_sensor_event ev;
    while (k_msgq_get(&peripheral_sensor_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger sensor change for %d", ev.sensor_index);
        raise_zmk_sensor_event(ev);
    }
}

K_WORK_DEFINE(peripheral_sensor_event_work, peripheral_sensor_event_work_callback);

void zmk_sensor_event_handle(struct zmk_sensor_event *ev) {
    k_msgq_put(&peripheral_sensor_event_msgq, ev, K_NO_WAIT);
    k_work_submit(&peripheral_sensor_event_work);
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */

K_THREAD_STACK_DEFINE(split_central_split_run_q_stack,
                      CONFIG_ZMK_SPLIT_CENTRAL_SPLIT_RUN_STACK_SIZE);

struct k_work_q split_central_split_run_q;

K_MSGQ_DEFINE(zmk_split_central_split_run_msgq,
              sizeof(struct zmk_split_run_behavior_payload_wrapper),
              CONFIG_ZMK_SPLIT_CENTRAL_SPLIT_RUN_QUEUE_SIZE, 4);

void split_central_split_run_callback(struct k_work *work) {
    struct zmk_split_run_behavior_payload_wrapper payload_wrapper;

    LOG_DBG("");

    while (k_msgq_get(&zmk_split_central_split_run_msgq, &payload_wrapper, K_NO_WAIT) == 0) {
        send_split_run_impl(&payload_wrapper);
    }
}

K_WORK_DEFINE(split_central_split_run_work, split_central_split_run_callback);

static int
split_invoke_behavior_payload(struct zmk_split_run_behavior_payload_wrapper payload_wrapper) {
    LOG_DBG("");

    int err = k_msgq_put(&zmk_split_central_split_run_msgq, &payload_wrapper, K_MSEC(100));
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Consumer message queue full, popping first message and queueing again");
            struct zmk_split_run_behavior_payload_wrapper discarded_report;
            k_msgq_get(&zmk_split_central_split_run_msgq, &discarded_report, K_NO_WAIT);
            return split_invoke_behavior_payload(payload_wrapper);
        }
        default:
            LOG_WRN("Failed to queue behavior to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&split_central_split_run_q, &split_central_split_run_work);

    return 0;
};

int zmk_split_invoke_behavior(uint8_t source, struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event, bool state) {
    struct zmk_split_run_behavior_payload payload = {.data = {
                                                         .param1 = binding->param1,
                                                         .param2 = binding->param2,
                                                         .position = event.position,
                                                         .state = state ? 1 : 0,
                                                     }};
    const size_t payload_dev_size = sizeof(payload.behavior_dev);
    if (strlcpy(payload.behavior_dev, binding->behavior_dev, payload_dev_size) >=
        payload_dev_size) {
        LOG_ERR("Truncated behavior label %s to %s before invoking peripheral behavior",
                binding->behavior_dev, payload.behavior_dev);
    }

    struct zmk_split_run_behavior_payload_wrapper wrapper = {.source = source, .payload = payload};
    return split_invoke_behavior_payload(wrapper);
}

static int zmk_split_central_init(void) {
    k_work_queue_start(&split_central_split_run_q, split_central_split_run_q_stack,
                       K_THREAD_STACK_SIZEOF(split_central_split_run_q_stack),
                       CONFIG_ZMK_SPLIT_CENTRAL_PRIORITY, NULL);
    return 0;
}

SYS_INIT(zmk_split_central_init, APPLICATION, CONFIG_ZMK_SPLIT_INIT_PRIORITY);
