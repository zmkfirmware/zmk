#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/retention/retention.h>
#include <zephyr/retention/bootmode.h>

static int dbl_tap_boot_mode_init(void) {
    bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);

    k_busy_wait(CONFIG_ZMK_DBL_TAP_BOOTLOADER_TIMEOUT * 1000);

    bootmode_clear();

    return 0;
}

SYS_INIT(dbl_tap_boot_mode_init, PRE_KERNEL_2, 20);