/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <kernel.h>
#include <power/power.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/usb.h>
#include <zmk/activity.h>

bool is_usb_power_present() {
#ifdef CONFIG_USB
    return zmk_usb_is_powered();
#else
    return false;
#endif /* CONFIG_USB */
}

enum power_states sys_pm_policy_next_state(int32_t ticks) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP_STATES
#ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1
    if (zmk_activity_get_state() == ZMK_ACTIVITY_SLEEP && !is_usb_power_present()) {
        return SYS_POWER_STATE_DEEP_SLEEP_1;
    }
#endif /* CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1 */
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP_STATES */

    return SYS_POWER_STATE_ACTIVE;
}

bool sys_pm_policy_low_power_devices(enum power_states pm_state) {
    return sys_pm_is_sleep_state(pm_state);
}