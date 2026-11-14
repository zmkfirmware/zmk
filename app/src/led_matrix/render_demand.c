/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* Included by render.c. Track potential future output, before later sources
 * overwrite both pixels and these flags in the same order as composition. */
static bool led_matrix_renderer_color_needs_time(uint32_t color, uint8_t brightness,
                                                 uint8_t renderer) {
    bool animated = renderer != LED_MATRIX_RENDERER_SOLID;
    if ((renderer == LED_MATRIX_RENDERER_SPECTRUM || renderer == LED_MATRIX_RENDERER_SWIRL) &&
        !led_matrix_color_is_dynamic(color)) {
        struct led_hsb hsb = rgb8_to_hsb(color24_to_rgb8(color));
        /* Hue rotation cannot change gray, and RGB->HSB may quantize a very
         * faint configured color to black before profile/global brightness. */
        if (hsb.s == 0U || hsb.b == 0U) {
            return false;
        }
        hsb.h = 0U;
        struct led_rgb8 peak = hsb_to_rgb8(hsb);
        color = ((uint32_t)peak.r << 16) | ((uint32_t)peak.g << 8) | peak.b;
    }
    return led_matrix_color_needs_time(color, brightness, animated);
}

static bool led_matrix_keypress_release_pending(size_t position, uint32_t now_ms) {
    uint32_t deadline = keypress_release_deadline_ms[position];
    if (deadline == 0U) {
        return false;
    }
    if ((int32_t)(now_ms - deadline) >= 0) {
        keypress_release_fade_start_ms[position] = 0U;
        keypress_release_deadline_ms[position] = 0U;
        return false;
    }
    return true;
}

static void led_matrix_mark_keypress_time(const struct led_matrix_profile_cfg *profile,
                                          uint32_t now_ms) {
    uint32_t range = profile->keypress_range > 0 ? (uint32_t)profile->keypress_range : 0U;
    for (size_t position = 0; position < ZMK_KEYMAP_LEN; position++) {
        bool release_pending = led_matrix_keypress_release_pending(position, now_ms);
        if (!pressed_positions[position] && !release_pending) {
            continue;
        }
        uint16_t origin_led = position_to_led[position];
        uint16_t origin_cell = position_to_cell[position];
        if (origin_led == LM_NOLED || origin_led >= LOGICAL_LED_COUNT || origin_cell == LM_NOLED ||
            origin_cell >= grid_cell_count) {
            continue;
        }
        uint32_t color = led_matrix_configured_color(profile->color_map, profile->color_map_count,
                                                     LM_BLACK, origin_led);
        if (!led_matrix_color_needs_time(color, profile->brightness,
                                         !pressed_positions[position] && release_pending)) {
            continue;
        }
        for (size_t cell = 0; cell < grid_cell_count; cell++) {
            uint16_t led = cell_to_led[cell];
            if (led != LM_NOLED && led < LOGICAL_LED_COUNT &&
                keypress_range_intensity(range, cell_chebyshev_distance(origin_cell, cell)) > 0U) {
                logical_leds_dynamic[led] = true;
            }
        }
    }
}

static void led_matrix_mark_profile_time(const struct led_matrix_profile_cfg *profile,
                                         uint32_t now_ms) {
    if (profile->renderer == LED_MATRIX_RENDERER_KEYPRESS) {
        led_matrix_mark_keypress_time(profile, now_ms);
        return;
    }
    if (profile->renderer == LED_MATRIX_RENDERER_RIPPLE) {
        if (!ripple_state.active || grid_cell_count == 0U || grid_row_count == 0U ||
            grid_col_count == 0U || ripple_state.origin_cell >= grid_cell_count) {
            ripple_state.active = false;
            return;
        }
        uint32_t radius =
            (now_ms - ripple_state.start_time_ms) / ripple_step_ms_for_profile(profile);
        uint16_t max_radius = (grid_row_count - 1U) + (grid_col_count - 1U);
        if (radius > max_radius + RIPPLE_TAIL_CELLS) {
            ripple_state.active = false;
            return;
        }
        uint16_t origin_row = cell_row(ripple_state.origin_cell);
        uint16_t origin_col = cell_col(ripple_state.origin_cell);
        for (size_t cell = 0; cell < grid_cell_count; cell++) {
            uint16_t led = cell_to_led[cell];
            if (led == LM_NOLED || led >= LOGICAL_LED_COUNT) {
                continue;
            }
            uint16_t row = cell_row(cell), col = cell_col(cell);
            uint16_t distance = (row > origin_row ? row - origin_row : origin_row - row) +
                                (col > origin_col ? col - origin_col : origin_col - col);
            uint32_t color = led_matrix_configured_color(profile->color_map,
                                                         profile->color_map_count, LM_BLACK, led);
            /* A pixel already behind the tail can no longer change in this ripple. */
            logical_leds_dynamic[led] |=
                radius <= distance + RIPPLE_TAIL_CELLS &&
                led_matrix_color_needs_time(color, profile->brightness, true);
        }
        return;
    }

    if (profile->renderer == LED_MATRIX_RENDERER_RAIN) {
        if (grid_cell_count == 0U || grid_row_count == 0U || grid_col_count == 0U) {
            return;
        }
        for (size_t cell = 0; cell < grid_cell_count; cell++) {
            uint16_t led = cell_to_led[cell];
            if (led != LM_NOLED && led < LOGICAL_LED_COUNT) {
                uint32_t color = led_matrix_configured_color(
                    profile->color_map, profile->color_map_count, LM_BLACK, led);
                logical_leds_dynamic[led] |=
                    led_matrix_color_needs_time(color, profile->brightness, true);
            }
        }
        return;
    }
    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        uint32_t color = led_matrix_configured_color(profile->color_map, profile->color_map_count,
                                                     LM_BLACK, led);
        logical_leds_dynamic[led] =
            led_matrix_renderer_color_needs_time(color, profile->brightness, profile->renderer);
    }
}

static void
led_matrix_mark_independent_time(const struct led_matrix_independent_group_cfg *group,
                                 const struct led_matrix_independent_profile_cfg *profile) {
    for (size_t i = 0; i < group->led_count; i++) {
        uint32_t color =
            led_matrix_configured_color(profile->color_map, profile->color_map_count, LM_BLACK, i);
        logical_leds_dynamic[group->leds[i]] =
            led_matrix_renderer_color_needs_time(color, profile->brightness, profile->renderer);
    }
}
