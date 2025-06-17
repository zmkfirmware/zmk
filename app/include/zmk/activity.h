/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

enum zmk_activity_state { ZMK_ACTIVITY_ACTIVE, ZMK_ACTIVITY_IDLE, ZMK_ACTIVITY_SLEEP };

enum zmk_activity_state zmk_activity_get_state(void);

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
void zmk_enable_sleep(void);
void zmk_disable_sleep(void);
void zmk_toggle_sleep(void);
#endif