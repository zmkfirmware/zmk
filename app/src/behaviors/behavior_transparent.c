/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_transparent

#include <device.h>
#include <power/reboot.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_transparent_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_TRANSPARENT;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_TRANSPARENT;
}

static const struct behavior_driver_api behavior_transparent_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

DEVICE_AND_API_INIT(behavior_transparent, DT_INST_LABEL(0), behavior_transparent_init, NULL, NULL,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_transparent_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */