/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/behavior.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

struct zmk_send_string_config {
    /// zmk,character-map driver instance to use
    const struct device *character_map;
    /// Time in milliseconds to wait between key presses
    uint32_t wait_ms;
    /// Time in milliseconds to wait between the press and release of each key
    uint32_t tap_ms;
};

/**
 * Assert at compile time that a zmk,charmap chosen node is set.
 */
#define ZMK_BUILD_ASSERT_HAS_CHOSEN_CHARMAP()                                                      \
    BUILD_ASSERT(                                                                                  \
        DT_HAS_CHOSEN(zmk_charmap),                                                                \
        "A zmk,charmap chosen node must be set to use send string functions. See "                 \
        "https://zmk.dev/docs/keymaps/behaviors/send-string#character-maps for more information.")

/**
 * Get a struct zmk_send_string_config which uses the zmk,charmap chosen node
 * and Kconfig options for timing.
 *
 * Use ZMK_BUILD_ASSERT_HAS_CHOSEN_CHARMAP() somewhere in the file before using
 * this macro to provide a nice error message if a character map hasn't been set.
 */
#define ZMK_SEND_STRING_CONFIG_DEFAULT                                                             \
    ((struct zmk_send_string_config){                                                              \
        .character_map = DEVICE_DT_GET(DT_CHOSEN(zmk_charmap)),                                    \
        .wait_ms = CONFIG_ZMK_SEND_STRING_DEFAULT_WAIT_MS,                                         \
        .tap_ms = CONFIG_ZMK_SEND_STRING_DEFAULT_TAP_MS,                                           \
    })

/**
 * Assert at compile time that a DT_DRV_INST(n) has a charmap property or a
 * zmk,character-map chosen node is set.
 */
#define ZMK_BUILD_ASSERT_DT_INST_HAS_CHARMAP(n)                                                    \
    BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, charmap) || DT_HAS_CHOSEN(zmk_charmap),                  \
                 "Node " DT_NODE_PATH(DT_DRV_INST(                                                 \
                     n)) " requires a charmap property or a zmk,charmap chosen node. "             \
                         "See https://zmk.dev/docs/keymaps/behaviors/send-string#character-maps "  \
                         "for more information.")

/**
 * Get a struct zmk_send_string_config from properties on DT_DRV_INST(n) with
 * fallbacks to the values from ZMK_SEND_STRING_CONFIG_DEFAULT.
 *
 * The driver should have the following properties defined in its YAML file:
 *
 *   charmap:
 *     type: phandle
 *   wait-ms:
 *     type: int
 *   tap-ms:
 *     type: int
 *
 * Use ZMK_BUILD_ASSERT_CHARACTER_MAP_DT_INST_PROP(n) somewhere in the file before using
 * this macro to provide a nice error message if a character map hasn't been set.
 */
#define ZMK_SEND_STRING_CONFIG_DT_INST_PROP(n)                                                     \
    ((struct zmk_send_string_config){                                                              \
        .character_map = DEVICE_DT_GET(DT_INST_PROP_OR(n, charmap, DT_CHOSEN(zmk_charmap))),       \
        .wait_ms = DT_INST_PROP_OR(n, wait_ms, CONFIG_ZMK_SEND_STRING_DEFAULT_WAIT_MS),            \
        .tap_ms = DT_INST_PROP_OR(n, tap_ms, CONFIG_ZMK_SEND_STRING_DEFAULT_TAP_MS),               \
    })

/**
 * Queues behaviors to type a string.
 *
 * @param config Character map and other configuration to use.
 * Pass &ZMK_SEND_STRING_CONFIG_DEFAULT to use default values.
 * @param position Key position to use for the key presses/releases
 * @param text UTF-8 encoded string
 */
void zmk_send_string(const struct zmk_send_string_config *config,
                     const struct zmk_behavior_binding_event *event, const char *text);
