/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_soft_poweroff
#define SLEEP_S 2u

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/activity.h>

#include <dt-bindings/zmk/soft_poweroff.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_softoff_config {
    int type;
};

static void enter_deep_sleep() {
    // Turn off the system. See `zephyr/samples/boards/nrf/system_off` or `app/src/activity.c`.
    // Equivalent to ACPI S5.
    // Put devices in suspend power mode before sleeping.
    zmk_activity_set_state(ZMK_ACTIVITY_SLEEP);
    pm_state_force(0u, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
    k_sleep(K_SECONDS(SLEEP_S));

    // Normally the code below will not be reached.
    pm_state_force(0u, &(struct pm_state_info){PM_STATE_ACTIVE, 0, 0});
    zmk_activity_set_state(ZMK_ACTIVITY_ACTIVE);
    LOG_WRN("The keyboard is not powered off!");
}

static void enter_deep_sleep_process(struct k_work *item) { enter_deep_sleep(); }

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
K_WORK_DELAYABLE_DEFINE(enter_deep_sleep_work, enter_deep_sleep_process);
#endif

static int behavior_softoff_init(const struct device *dev) { return 0; };

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
#if IS_ENABLED(CONFIG_ZMK_SLEEP)
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_softoff_config *cfg = dev->config;

    switch (cfg->type) {
    case LOCKED:
        // sleep before kscan interrupts enabled, so keys are locked
        enter_deep_sleep();
        break;
    case SLEEP:
        // sleep after kscan interrupts enabled
        // so the keyboard can be waked up by typing(on platforms with PORT events, gpiote,
        // interrupts enabled) limitation: any type before the actual sleep will make it a poweroff
        // behavior how to improve: introduce something to terminate the kscan(but keep the
        // interrupts) after sleep requested
        k_work_reschedule(&enter_deep_sleep_work, K_SECONDS(SLEEP_S));
        break;
    default:
        break;
    }
#endif

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_softoff_driver_api = {
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

#define SOFTOFF_INST(n)                                                                            \
    static const struct behavior_softoff_config behavior_softoff_config_##n = {                    \
        .type = DT_INST_PROP(n, type)};                                                            \
    DEVICE_DT_INST_DEFINE(n, behavior_softoff_init, NULL, NULL, &behavior_softoff_config_##n,      \
                          APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                          &behavior_softoff_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SOFTOFF_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
