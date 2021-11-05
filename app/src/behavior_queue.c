/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/behavior_queue.h>

#include <kernel.h>
#include <logging/log.h>
#include <drivers/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct q_item {
    uint32_t position;
    struct zmk_behavior_binding binding;
    bool press : 1;
    uint32_t wait : 31;
};

K_MSGQ_DEFINE(zmk_behavior_queue_msgq, sizeof(struct q_item), CONFIG_ZMK_BEHAVIORS_QUEUE_SIZE, 4);

static void behavior_queue_process_next(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(queue_work, behavior_queue_process_next);

static void behavior_queue_process_next(struct k_work *work) {
    struct q_item item = {.wait = 0};

    while (k_msgq_get(&zmk_behavior_queue_msgq, &item, K_NO_WAIT) == 0) {
        LOG_DBG("Invoking %s: 0x%02x 0x%02x", log_strdup(item.binding.behavior_dev),
                item.binding.param1, item.binding.param2);

        struct zmk_behavior_binding_event event = {.position = item.position,
                                                   .timestamp = k_uptime_get()};

        if (item.press) {
            behavior_keymap_binding_pressed(&item.binding, event);
        } else {
            behavior_keymap_binding_released(&item.binding, event);
        }

        LOG_DBG("Processing next queued behavior in %dms", item.wait);

        if (item.wait > 0) {
            k_work_schedule(&queue_work, K_MSEC(item.wait));
            break;
        }
    }
}

int zmk_behavior_queue_add(uint32_t position, const struct zmk_behavior_binding binding, bool press,
                           uint32_t wait) {
    struct q_item item = {.press = press, .binding = binding, .wait = wait};

    const int ret = k_msgq_put(&zmk_behavior_queue_msgq, &item, K_NO_WAIT);
    if (ret < 0) {
        return ret;
    }

    if (!k_work_delayable_is_pending(&queue_work)) {
        behavior_queue_process_next(&queue_work.work);
    }

    return 0;
}
