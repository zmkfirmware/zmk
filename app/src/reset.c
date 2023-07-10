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

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// AdaFruit nrf52 Bootloader Specific. See
// https://github.com/adafruit/Adafruit_nRF52_Bootloader/blob/d6b28e66053eea467166f44875e3c7ec741cb471/src/main.c#L107
#define ADAFRUIT_MAGIC_UF2 0x57

FUNC_NORETURN void zmk_reset(int type) {
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
}

void zmk_reset_settings(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE)
    zmk_ble_unpair_all();
#endif
    // TODO: clear settings for all subsystems.
}