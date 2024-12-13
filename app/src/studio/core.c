/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/studio/core.h>

ZMK_EVENT_IMPL(zmk_studio_core_lock_state_changed);

static enum zmk_studio_core_lock_state state = IS_ENABLED(CONFIG_ZMK_STUDIO_LOCKING)
                                                   ? ZMK_STUDIO_CORE_LOCK_STATE_LOCKED
                                                   : ZMK_STUDIO_CORE_LOCK_STATE_UNLOCKED;

enum zmk_studio_core_lock_state zmk_studio_core_get_lock_state(void) { return state; }

static void set_state(enum zmk_studio_core_lock_state new_state) {
    if (state == new_state) {
        return;
    }

    state = new_state;

    raise_zmk_studio_core_lock_state_changed(
        (struct zmk_studio_core_lock_state_changed){.state = state});
}

#if CONFIG_ZMK_STUDIO_LOCK_IDLE_TIMEOUT_SEC > 0

static void core_idle_lock_timeout_cb(struct k_work *work) { zmk_studio_core_lock(); }

K_WORK_DELAYABLE_DEFINE(core_idle_lock_timeout, core_idle_lock_timeout_cb);

void zmk_studio_core_reschedule_lock_timeout() {
    k_work_reschedule(&core_idle_lock_timeout, K_SECONDS(CONFIG_ZMK_STUDIO_LOCK_IDLE_TIMEOUT_SEC));
}

#else

void zmk_studio_core_reschedule_lock_timeout() {}

#endif

void zmk_studio_core_unlock() {
    set_state(ZMK_STUDIO_CORE_LOCK_STATE_UNLOCKED);

    zmk_studio_core_reschedule_lock_timeout();
}

void zmk_studio_core_lock() { set_state(ZMK_STUDIO_CORE_LOCK_STATE_LOCKED); }