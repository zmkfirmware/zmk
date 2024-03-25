/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/character_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <zmk/send_string.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/**
 * Reads a codepoint from the given UTF-8 string and advanced the string pointer
 * to the next character.
 *
 * This assumes the input string is valid UTF-8. Behavior on ill-formed input
 * is undefined.
 *
 * Based on public domain code at https://gist.github.com/tylerneylon/9773800
 */
static uint32_t decode_utf8(const char **str) {
    int size = **str ? u32_count_leading_zeros(~(**str << 24)) : 0;
    uint32_t mask = (1 << (8 - size)) - 1;
    uint32_t codepoint = **str & mask;

    for (++(*str), --size; size > 0 && **str; --size, ++(*str)) {
        codepoint <<= 6;
        codepoint += (**str & 0x3F);
    }

    return codepoint;
}

void zmk_send_string(const struct zmk_send_string_config *config, uint32_t position,
                     const char *text) {
    const char *current = text;

    uint32_t codepoint;
    while ((codepoint = decode_utf8(&current))) {
        struct zmk_behavior_binding binding;
        int ret = character_map_codepoint_to_binding(config->character_map, codepoint, &binding);

        if (ret != 0) {
            LOG_WRN("Failed to map codepoint 0x%04x to a behavior binding: %d", codepoint, ret);
            continue;
        }

        zmk_behavior_queue_add(position, binding, true, config->tap_ms);
        zmk_behavior_queue_add(position, binding, false, config->wait_ms);
    }
}