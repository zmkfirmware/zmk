/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <devicetree.h>

#if IS_ENABLED(CONFIG_ZMK_MOUSE_WORK_QUEUE_DEDICATED)

K_THREAD_STACK_DEFINE(mouse_work_stack_area, CONFIG_ZMK_MOUSE_DEDICATED_THREAD_STACK_SIZE);

static struct k_work_q mouse_work_q;

struct k_work_q *zmk_mouse_work_q() {
    return &mouse_work_q;
}

#else

struct k_work_q *zmk_mouse_work_q() {
    return &k_sys_work_q;
}

#endif

int zmk_mouse_init() {
#if IS_ENABLED(CONFIG_ZMK_MOUSE_WORK_QUEUE_DEDICATED)
    k_work_q_start(&mouse_work_q, mouse_work_stack_area,
                   K_THREAD_STACK_SIZEOF(mouse_work_stack_area),
                   CONFIG_ZMK_MOUSE_DEDICATED_THREAD_PRIORITY);
#endif
    return 0;
}