/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sleep

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/behavior.h>
#include <zmk/activity.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int behavior_sleep_init(const struct device *dev) { return 0; };

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return activity_set_state(ZMK_ACTIVITY_SLEEP);
}

static const struct behavior_driver_api behavior_sleep_driver_api = {
    .binding_released = on_keymap_binding_released,
};

DEVICE_AND_API_INIT(behavior_sleep, DT_INST_LABEL(0), behavior_sleep_init, NULL, NULL, APPLICATION,
                    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_sleep_driver_api);