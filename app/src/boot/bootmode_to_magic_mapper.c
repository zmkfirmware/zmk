/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_bootmode_to_magic_mapper

#include <zephyr/device.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/retention/retention.h>
#include <zephyr/drivers/retained_mem.h>

static const struct device *magic_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_magic_boot_mode));
#define MAGIC_DEST_ONE_BYTE (DT_REG_SIZE(DT_CHOSEN(zmk_magic_boot_mode)) == 1)

#if MAGIC_DEST_ONE_BYTE
typedef uint8_t magic_val_t;
#else
typedef uint32_t magic_val_t;
#endif

static const magic_val_t bootloader_magic_value = CONFIG_ZMK_BOOTMODE_BOOTLOADER_MAGIC_VALUE;

static ssize_t btmm_ram_size(const struct device *dev) { return (ssize_t)1; }

static int btmm_ram_read(const struct device *dev, off_t offset, uint8_t *buffer, size_t size) {
    if (size != 1) {
        return -ENOTSUP;
    }

    magic_val_t val;
    int ret = retention_read(magic_dev, 0, (uint8_t *)&val, sizeof(val));
    if (ret < 0) {
        return ret;
    }

    *buffer = (val == bootloader_magic_value) ? BOOT_MODE_TYPE_BOOTLOADER : BOOT_MODE_TYPE_NORMAL;

    return 0;
}

static int btmm_ram_write(const struct device *dev, off_t offset, const uint8_t *buffer,
                          size_t size) {
    if (size != 1) {
        return -ENOTSUP;
    }

    magic_val_t val = (*buffer == BOOT_MODE_TYPE_BOOTLOADER) ? bootloader_magic_value : 0;

    return retention_write(magic_dev, 0, (uint8_t *)&val, sizeof(val));
}

static int btmm_ram_clear(const struct device *dev) { return retention_clear(magic_dev); }

static const struct retained_mem_driver_api btmm_api = {
    .size = btmm_ram_size,
    .read = btmm_ram_read,
    .write = btmm_ram_write,
    .clear = btmm_ram_clear,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, 0, &btmm_api);
