/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/behavior_queue.h>
#include <zmk/behavior.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct q_item {
    uint32_t position;
    const char *behavior_dev;
    uint32_t param1;
    uint32_t param2;
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    uint8_t source;
#endif
    bool press;
    uint32_t wait : 31;
};

K_MSGQ_DEFINE(zmk_behavior_queue_msgq, sizeof(struct q_item), CONFIG_ZMK_BEHAVIORS_QUEUE_SIZE, 4);

static void behavior_queue_process_next(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(queue_work, behavior_queue_process_next);

static void behavior_queue_process_next(struct k_work *work) {
    struct q_item item = {.wait = 0};
    int ret;

    while (k_msgq_get(&zmk_behavior_queue_msgq, &item, K_NO_WAIT) == 0) {
        LOG_DBG("Invoking %s: 0x%02x 0x%02x", item.behavior_dev, item.param1, item.param2);

        struct zmk_behavior_binding_event event = {
            .behavior_dev = item.behavior_dev,
            .param1 = item.param1,
            .param2 = item.param2,
            .position = item.position,
            .timestamp = k_uptime_get(),
            .type = item.press ? ZMK_BEHAVIOR_TRIG_TYPE_PRESS : ZMK_BEHAVIOR_TRIG_TYPE_RELEASE,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
            .source = item.source
#endif
        };
        ret = raise_zmk_behavior_binding_event(event);
        if (ret < 0) {
            LOG_ERR("Error %d occurred while processing behavior in queue.", ret);
        }
        LOG_DBG("Processing next queued behavior in %dms", item.wait);

        if (item.wait > 0) {
            k_work_schedule(&queue_work, K_MSEC(item.wait));
            break;
        }
    }
}

int zmk_behavior_queue_add(const struct zmk_behavior_binding_event *event, bool press,
                           uint32_t wait) {
    struct q_item item = {
        .press = press,
        .behavior_dev = event->behavior_dev,
        .param1 = event->param1,
        .param2 = event->param2,
        .wait = wait,
        .position = event->position,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = event->source,
#endif
    };

    const int ret = k_msgq_put(&zmk_behavior_queue_msgq, &item, K_NO_WAIT);
    if (ret < 0) {
        return ret;
    }

    if (!k_work_delayable_is_pending(&queue_work)) {
        behavior_queue_process_next(&queue_work.work);
    }

    return 0;
}
