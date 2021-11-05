/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <kernel.h>
#include <pm/pm.h>

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

struct pm_state_info pm_policy_next_state(int32_t ticks) {
    if (zmk_activity_get_state() == ZMK_ACTIVITY_SLEEP && !is_usb_power_present()) {
        return (struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0};
    }

    return (struct pm_state_info){PM_STATE_ACTIVE, 0, 0};
}
