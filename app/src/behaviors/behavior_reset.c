/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_reset

#include <device.h>
#include <power/reboot.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
struct behavior_reset_config {
    int type;
};

static int behavior_reset_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_reset_config *cfg = dev->config;

    // TODO: Correct magic code for going into DFU?
    // See
    // https://github.com/adafruit/Adafruit_nRF52_Bootloader/blob/d6b28e66053eea467166f44875e3c7ec741cb471/src/main.c#L107
    sys_reboot(cfg->type);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_reset_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .locality = BEHAVIOR_LOCALITY_EVENT_SOURCE,
};

#define RST_INST(n)                                                                                \
    static const struct behavior_reset_config behavior_reset_config_##n = {                        \
        .type = DT_INST_PROP(n, type)};                                                            \
    DEVICE_DT_INST_DEFINE(n, behavior_reset_init, device_pm_control_nop, NULL,                     \
                          &behavior_reset_config_##n, APPLICATION,                                 \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_reset_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RST_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
