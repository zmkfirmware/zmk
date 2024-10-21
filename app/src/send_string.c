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

static bool is_utf8_continuation(const char c) { return (c & 0xc0) == 0x80; }

/**
 * Reads a code point from the UTF-8 string at **str and advances *str to point
 * to the start of the next code point.
 *
 * The string is expected to be valid UTF-8. No validation is performed.
 *
 * Based on public domain code at https://gist.github.com/tylerneylon/9773800
 */
static uint32_t decode_utf8(const char **str) {
    int size = **str ? u32_count_leading_zeros(~(**str << 24)) : 0; // Number of leading 1 bits
    uint32_t mask = (1 << (8 - size)) - 1; // All 1s with "size" leading 0s.
    uint32_t codepoint = **str & mask;

    for (++(*str), --size; size > 0 && is_utf8_continuation(**str); --size, ++(*str)) {
        codepoint <<= 6;
        codepoint += (**str & 0x3F);
    }

    return codepoint;
}

void zmk_send_string(const struct zmk_send_string_config *config,
                     const struct zmk_behavior_binding_event *event, const char *text) {
    const char *current = text;

    uint32_t codepoint;
    while ((codepoint = decode_utf8(&current))) {
        struct zmk_behavior_binding binding;
        int ret = character_map_codepoint_to_binding(config->character_map, codepoint, &binding);

        if (ret != 0) {
            LOG_WRN("Failed to map codepoint 0x%04x to a behavior binding: %d", codepoint, ret);
            continue;
        }

        zmk_behavior_queue_add(event, binding, true, config->tap_ms);
        zmk_behavior_queue_add(event, binding, false, config->wait_ms);
    }
}