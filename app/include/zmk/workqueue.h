/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

struct k_work_q *zmk_main_work_q(void);

void zmk_main_work_queue_run(void);

struct k_work_q *zmk_workqueue_lowprio_work_q(void);
