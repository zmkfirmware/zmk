/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/sys/util.h>

struct zmk_boot_magic_combo_config {
    const uint16_t *combo_positions;
    uint8_t combo_positions_len;
    bool jump_to_bootloader;
    bool reset_settings;
    bool *state;
};

#define ZMK_BOOT_MAGIC_COMBO_CONFIG_NAME(node_id) _CONCAT(zmk_boot_magic_combo_config_, node_id)

#define ZMK_BOOT_MAGIC_COMBO_CONFIG_DECLARE(node_id)                                               \
    extern const struct zmk_boot_magic_combo_config ZMK_BOOT_MAGIC_COMBO_CONFIG_NAME(node_id)
