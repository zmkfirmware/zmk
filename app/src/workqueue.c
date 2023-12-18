/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zmk/workqueue.h>

K_THREAD_STACK_DEFINE(lowprio_q_stack, CONFIG_ZMK_LOW_PRIORITY_THREAD_STACK_SIZE);

static struct k_work_q lowprio_work_q;

struct k_work_q *zmk_workqueue_lowprio_work_q(void) {
    return &lowprio_work_q;
}

static int workqueue_init(void) {
    static const struct k_work_queue_config queue_config = {.name = "Low Priority Work Queue"};
    k_work_queue_start(&lowprio_work_q, lowprio_q_stack, K_THREAD_STACK_SIZEOF(lowprio_q_stack),
                       CONFIG_ZMK_LOW_PRIORITY_THREAD_PRIORITY, &queue_config);
    return 0;
}

SYS_INIT(workqueue_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
