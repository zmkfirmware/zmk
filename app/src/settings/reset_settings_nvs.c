/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// SETTINGS_PARTITION must match settings_nvs.c
#if DT_HAS_CHOSEN(zephyr_settings_partition)
#define SETTINGS_PARTITION DT_FIXED_PARTITION_ID(DT_CHOSEN(zephyr_settings_partition))
#else
#define SETTINGS_PARTITION FIXED_PARTITION_ID(storage_partition)
#endif

int zmk_settings_erase(void) {
    LOG_INF("Erasing settings flash partition");

    const struct flash_area *fa;
    int rc = flash_area_open(SETTINGS_PARTITION, &fa);
    if (rc) {
        LOG_ERR("Failed to open settings flash: %d", rc);
        return rc;
    }

    rc = flash_area_erase(fa, 0, fa->fa_size);
    if (rc) {
        LOG_ERR("Failed to erase settings flash: %d", rc);
    }

    flash_area_close(fa);

    return rc;
}
