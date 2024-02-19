/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/event_manager.h>

enum zmk_studio_core_lock_state {
    ZMK_STUDIO_CORE_LOCK_STATE_LOCKED = 0,
    ZMK_STUDIO_CORE_LOCK_STATE_UNLOCKED = 1,
};

struct zmk_studio_core_lock_state_changed {
    enum zmk_studio_core_lock_state state;
};

struct zmk_studio_core_unlock_requested {};

ZMK_EVENT_DECLARE(zmk_studio_core_lock_state_changed);

enum zmk_studio_core_lock_state zmk_studio_core_get_lock_state(void);

void zmk_studio_core_unlock();
void zmk_studio_core_lock();
void zmk_studio_core_initiate_unlock();
void zmk_studio_core_complete_unlock();

void zmk_studio_core_reschedule_lock_timeout();