/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#endif
#include <zmk/reset.h>

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)

#include <zephyr/retention/bootmode.h>

#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// AdaFruit nrf52 Bootloader Specific. See
// https://github.com/adafruit/Adafruit_nRF52_Bootloader/blob/d6b28e66053eea467166f44875e3c7ec741cb471/src/main.c#L107
#define ADAFRUIT_MAGIC_UF2 0x57

void zmk_reset(int type) {
#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
    int ret = bootmode_set(type);
    if (ret < 0) {
        LOG_ERR("Failed to set the bootloader mode (%d)", ret);
    } else {
        sys_reboot(SYS_REBOOT_WARM);
    }
#else
    switch (type) {
    case ZMK_RESET_WARM:
        sys_reboot(SYS_REBOOT_WARM);
        break;

    case ZMK_RESET_COLD:
        sys_reboot(SYS_REBOOT_COLD);
        break;

    case ZMK_RESET_BOOTLOADER:
        // TODO: Add support for other types of bootloaders.
        sys_reboot(ADAFRUIT_MAGIC_UF2);
        break;

    default:
        LOG_ERR("Unknown reset type %d", type);
        break;
    }
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */
}

void zmk_reset_settings(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE)
    zmk_ble_unpair_all();
#endif
    // TODO: clear settings for all subsystems.
}
