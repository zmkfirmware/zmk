/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_reset

#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)

#include <zephyr/retention/bootmode.h>

#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
struct behavior_reset_config {
#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
    enum BOOT_MODE_TYPES boot_mode;
#else
    int type;
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_reset_config *cfg = dev->config;

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
    int ret = bootmode_set(cfg->boot_mode);
    if (ret < 0) {
        LOG_ERR("Failed to set the bootloader mode (%d)", ret);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    sys_reboot(SYS_REBOOT_WARM);
#else
    // See
    // https://github.com/adafruit/Adafruit_nRF52_Bootloader/blob/d6b28e66053eea467166f44875e3c7ec741cb471/src/main.c#L107
    sys_reboot(cfg->type);
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_reset_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .locality = BEHAVIOR_LOCALITY_EVENT_SOURCE,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define RST_INST(n)                                                                                \
    static const struct behavior_reset_config behavior_reset_config_##n = {                        \
        COND_CODE_1(                                                                               \
            IS_ENABLED(CONFIG_RETENTION_BOOT_MODE),                                                \
            (DT_INST_PROP(n, bootloader) ? BOOT_MODE_TYPE_BOOTLOADER : BOOT_MODE_TYPE_NORMAL),     \
            (.type = DT_INST_PROP(n, type))),                                                      \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_reset_config_##n, POST_KERNEL,          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_reset_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RST_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
