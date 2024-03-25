/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/poweroff.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/endpoints.h>

// Reimplement some of the device work from Zephyr PM to work with the new `sys_poweroff` API.
// TODO: Tweak this to smarter runtime PM of subsystems on sleep.

#ifdef CONFIG_ZMK_PM_DEVICE_SUSPEND_RESUME
TYPE_SECTION_START_EXTERN(const struct device *, pm_device_slots);

#if !defined(CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE)
/* Number of devices successfully suspended. */
static size_t zmk_num_susp;

int zmk_pm_suspend_devices(void) {
    const struct device *devs;
    size_t devc;

    devc = z_device_get_all_static(&devs);

    zmk_num_susp = 0;

    for (const struct device *dev = devs + devc - 1; dev >= devs; dev--) {
        int ret;

        /*
         * Ignore uninitialized devices, busy devices, wake up sources, and
         * devices with runtime PM enabled.
         */
        if (!device_is_ready(dev) || pm_device_is_busy(dev) || pm_device_state_is_locked(dev) ||
            pm_device_wakeup_is_enabled(dev) || pm_device_runtime_is_enabled(dev)) {
            continue;
        }

        ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
        /* ignore devices not supporting or already at the given state */
        if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY)) {
            continue;
        } else if (ret < 0) {
            LOG_ERR("Device %s did not enter %s state (%d)", dev->name,
                    pm_device_state_str(PM_DEVICE_STATE_SUSPENDED), ret);
            return ret;
        }

        TYPE_SECTION_START(pm_device_slots)[zmk_num_susp] = dev;
        zmk_num_susp++;
    }

    return 0;
}

void zmk_pm_resume_devices(void) {
    for (int i = (zmk_num_susp - 1); i >= 0; i--) {
        pm_device_action_run(TYPE_SECTION_START(pm_device_slots)[i], PM_DEVICE_ACTION_RESUME);
    }

    zmk_num_susp = 0;
}
#endif /* !CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE */
#endif /* CONFIG_ZMK_PM_DEVICE_SUSPEND_RESUME */

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
    // Need to sleep to give any other threads a chance so submit endpoint data.
    k_sleep(K_MSEC(100));
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

    int err = zmk_pm_suspend_devices();
    if (err < 0) {
        zmk_pm_resume_devices();
        return err;
    }

    LOG_DBG("soft-off: go to sleep");
    sys_poweroff();
    return 0;
}

#endif // IS_ENABLED(CONFIG_ZMK_PM_SOFT_OFF)