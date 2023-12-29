/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/endpoints.h>

#if IS_ENABLED(CONFIG_ZMK_PM_SOFT_OFF)

#define HAS_WAKERS DT_HAS_COMPAT_STATUS_OKAY(zmk_soft_off_wakeup_sources)

#if HAS_WAKERS

#define DEVICE_WITH_SEP(node_id, prop, idx) DEVICE_DT_GET(DT_PROP_BY_IDX(node_id, prop, idx)),

const struct device *soft_off_wakeup_sources[] = {
    DT_FOREACH_PROP_ELEM(DT_INST(0, zmk_soft_off_wakeup_sources), wakeup_sources, DEVICE_WITH_SEP)};

#endif

int zmk_pm_soft_off(void) {
#if IS_ENABLED(CONFIG_PM_DEVICE)
    size_t device_count;
    const struct device *devs;

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zmk_endpoints_clear_current();
#endif

    device_count = z_device_get_all_static(&devs);

    // There may be some matrix/direct kscan devices that would be used for wakeup
    // from normal "inactive goes to sleep" behavior, so disable them as wakeup devices
    // and then suspend them so we're ready to take over setting up our system
    // and then putting it into an off state.
    LOG_DBG("soft-on-off pressed cb: suspend devices");
    for (int i = 0; i < device_count; i++) {
        const struct device *dev = &devs[i];

        if (pm_device_wakeup_is_enabled(dev)) {
            pm_device_wakeup_enable(dev, false);
        }
        pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
    }
#endif // IS_ENABLED(CONFIG_PM_DEVICE)

#if HAS_WAKERS
    for (int i = 0; i < ARRAY_SIZE(soft_off_wakeup_sources); i++) {
        const struct device *dev = soft_off_wakeup_sources[i];
        pm_device_wakeup_enable(dev, true);
        pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
    }
#endif // HAS_WAKERS

    LOG_DBG("soft-off: go to sleep");
    return pm_state_force(0U, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
}

#endif // IS_ENABLED(CONFIG_ZMK_PM_SOFT_OFF)