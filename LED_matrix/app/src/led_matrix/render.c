/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "internal.h"

static int flush_matrix(void);

static bool led_matrix_base_source_needs_periodic_refresh(uint32_t now_ms) {
    if (!led_matrix_base_render_enabled()) {
        return false;
    }

    const struct led_matrix_profile_cfg *profile = selected_profile_cfg();
    bool profile_needs = led_matrix_profile_needs_periodic_refresh(profile, now_ms);
    bool independent_needs = led_matrix_independent_groups_need_periodic_refresh(now_ms);

    if (layer_color.active && layer_color.cfg != NULL) {
        return profile_needs || independent_needs ||
               led_matrix_layer_color_needs_periodic_refresh();
    }

    return profile_needs || independent_needs;
}

static bool led_matrix_needs_periodic_refresh(uint32_t now_ms) {
    if (!led_matrix_render_enabled()) {
        return false;
    }

    return led_matrix_base_source_needs_periodic_refresh(now_ms) ||
           led_matrix_status_needs_periodic_refresh(now_ms);
}

static enum zmk_led_matrix_render_source led_matrix_effective_source(void) {
    uint8_t sources = current_status_sources();
    const struct led_matrix_profile_cfg *profile = selected_profile_cfg();

    if (sources != 0U && led_matrix_render_enabled()) {
        return ZMK_LED_MATRIX_RENDER_SOURCE_STATUS_OVERLAY;
    }

    if (!led_matrix_base_render_enabled()) {
        return ZMK_LED_MATRIX_RENDER_SOURCE_OFF;
    }

    if (layer_color.active && layer_color.cfg != NULL) {
        return ZMK_LED_MATRIX_RENDER_SOURCE_LAYER_COLOR;
    }

    if (profile != NULL) {
        return ZMK_LED_MATRIX_RENDER_SOURCE_PROFILE;
    }

    if (led_matrix_has_independent_groups()) {
        return ZMK_LED_MATRIX_RENDER_SOURCE_INDEPENDENT;
    }

    return ZMK_LED_MATRIX_RENDER_SOURCE_OFF;
}

static int led_matrix_raise_state_changed(uint8_t reason_mask) {
    if (reason_mask == 0U) {
        return 0;
    }

    return raise_zmk_led_matrix_state_changed(
        (struct zmk_led_matrix_state_changed){.reason_mask = reason_mask});
}

static void render_profile_solid(const struct led_matrix_profile_cfg *profile, uint32_t now_ms) {
    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        logical_leds[led] =
            scale_color(resolve_profile_color(profile, led, now_ms), profile->brightness);
    }
}

static void render_profile_breathe(const struct led_matrix_profile_cfg *profile, uint32_t now_ms) {
    uint8_t intensity = breathe_intensity(led_matrix_effective_profile_period_ms(profile), now_ms);

    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        struct led_rgb8 color = resolve_profile_color(profile, led, now_ms);
        color = scale_color(color, profile->brightness);
        logical_leds[led] = scale_color(color, intensity);
    }
}

static void render_profile_spectrum(const struct led_matrix_profile_cfg *profile, uint32_t now_ms) {
    uint16_t hue_offset =
        spectrum_hue_offset(led_matrix_effective_profile_period_ms(profile), now_ms);

    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        struct led_rgb8 color =
            shift_color_hue(resolve_profile_color(profile, led, now_ms), hue_offset);
        logical_leds[led] = scale_color(color, profile->brightness);
    }
}

static void render_profile_swirl(const struct led_matrix_profile_cfg *profile, uint32_t now_ms) {
    uint16_t base_offset =
        spectrum_hue_offset(led_matrix_effective_profile_period_ms(profile), now_ms);

    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        uint16_t led_offset =
            LOGICAL_LED_COUNT > 0U ? (uint16_t)((SPECTRUM_HUE_MAX * led) / LOGICAL_LED_COUNT) : 0U;
        struct led_rgb8 color = shift_color_hue(resolve_profile_color(profile, led, now_ms),
                                                (base_offset + led_offset) % 360U);
        logical_leds[led] = scale_color(color, profile->brightness);
    }
}

static void render_profile_keypress(const struct led_matrix_profile_cfg *profile, uint32_t now_ms) {
    uint32_t keypress_range = profile->keypress_range > 0 ? (uint32_t)profile->keypress_range : 0U;

    for (size_t position = 0; position < ZMK_KEYMAP_LEN; position++) {
        bool source_pressed = pressed_positions[position];
        uint8_t source_intensity =
            source_pressed
                ? 100U
                : keypress_release_intensity(keypress_release_fade_start_ms[position],
                                             keypress_release_deadline_ms[position], now_ms);

        if (source_intensity == 0U) {
            continue;
        }

        uint16_t origin_led = position_to_led[position];
        uint16_t origin_cell = position_to_cell[position];
        if (origin_led == LM_NOLED || origin_led >= LOGICAL_LED_COUNT || origin_cell == LM_NOLED ||
            origin_cell >= grid_cell_count) {
            continue;
        }

        struct led_rgb8 color = resolve_profile_color(profile, origin_led, now_ms);
        color = scale_color(color, profile->brightness);
        color = scale_color(color, source_intensity);

        for (size_t cell = 0; cell < grid_cell_count; cell++) {
            uint16_t target_led = cell_to_led[cell];
            if (target_led == LM_NOLED || target_led >= LOGICAL_LED_COUNT) {
                continue;
            }

            uint16_t dist = cell_chebyshev_distance(origin_cell, (uint16_t)cell);
            uint8_t intensity = keypress_range_intensity(keypress_range, dist);
            if (intensity == 0U) {
                continue;
            }

            struct led_rgb8 target_color = scale_color(color, intensity);
            logical_leds[target_led] = max_blend(logical_leds[target_led], target_color);
        }
    }
}

static void render_profile_ripple(const struct led_matrix_profile_cfg *profile, uint32_t now_ms) {
    if (!ripple_state.active || grid_cell_count == 0U || grid_row_count == 0U ||
        grid_col_count == 0U || ripple_state.origin_cell >= grid_cell_count) {
        ripple_state.active = false;
        return;
    }

    uint32_t step_ms = ripple_step_ms_for_profile(profile);
    uint16_t origin_row = cell_row(ripple_state.origin_cell);
    uint16_t origin_col = cell_col(ripple_state.origin_cell);
    uint32_t elapsed = now_ms - ripple_state.start_time_ms;
    uint32_t radius = elapsed / step_ms;
    uint16_t max_radius = (grid_row_count - 1U) + (grid_col_count - 1U);

    if (radius > (max_radius + RIPPLE_TAIL_CELLS)) {
        ripple_state.active = false;
        return;
    }

    for (size_t cell = 0; cell < grid_cell_count; cell++) {
        uint16_t led = cell_to_led[cell];
        if (led == LM_NOLED || led >= LOGICAL_LED_COUNT) {
            continue;
        }

        uint16_t row = cell_row((uint16_t)cell);
        uint16_t col = cell_col((uint16_t)cell);
        uint16_t row_dist = row > origin_row ? (row - origin_row) : (origin_row - row);
        uint16_t col_dist = col > origin_col ? (col - origin_col) : (origin_col - col);
        uint16_t dist = row_dist + col_dist;
        int delta = (int)dist - (int)radius;
        uint8_t intensity = 0U;

        if (delta == 0) {
            intensity = 100U;
        } else if (delta == 1 || delta == -1) {
            intensity = 40U;
        } else if (delta == -2) {
            intensity = 20U;
        }

        if (intensity == 0U) {
            continue;
        }

        struct led_rgb8 color = resolve_profile_color(profile, led, now_ms);
        color = scale_color(color, profile->brightness);
        color = scale_color(color, intensity);
        logical_leds[led] = max_blend(logical_leds[led], color);
    }
}

static void render_profile_rain(const struct led_matrix_profile_cfg *profile, uint32_t now_ms) {
    if (grid_cell_count == 0U || grid_row_count == 0U || grid_col_count == 0U) {
        return;
    }

    uint32_t period_ms = led_matrix_effective_profile_period_ms(profile);
    uint32_t travel_steps = (uint32_t)grid_row_count + (2U * RAIN_TAIL_CELLS);
    uint32_t step_ms = MAX((uint32_t)1U, period_ms / travel_steps);
    uint32_t current_step = now_ms / step_ms;

    rain_sync_state(current_step, step_ms);

    for (size_t cell = 0; cell < grid_cell_count; cell++) {
        uint16_t led = cell_to_led[cell];
        if (led == LM_NOLED || led >= LOGICAL_LED_COUNT) {
            continue;
        }

        uint16_t row = cell_row((uint16_t)cell);
        uint16_t col = cell_col((uint16_t)cell);
        const struct led_matrix_rain_column_state *column = &rain_state.columns[col];
        uint8_t intensity = 0U;
        uint32_t age_steps = column->age_x100 / 100U;
        uint32_t tail_delta;

        if (!column->active || row > age_steps) {
            continue;
        }

        tail_delta = age_steps - row;
        if (tail_delta >= column->streak_cells) {
            continue;
        }

        if (tail_delta == 0U) {
            intensity = 100U;
        } else if (tail_delta == 1U) {
            intensity = RAIN_TAIL_NEAR_INTENSITY;
        } else if (tail_delta == 2U) {
            intensity = RAIN_TAIL_FAR_INTENSITY;
        }

        if (intensity == 0U) {
            continue;
        }

        struct led_rgb8 color = resolve_profile_color(profile, led, now_ms);
        color = scale_color(color, profile->brightness);
        logical_leds[led] = max_blend(logical_leds[led], scale_color(color, intensity));
    }
}

static void render_profile_starlight(const struct led_matrix_profile_cfg *profile,
                                     uint32_t now_ms) {
    uint32_t base_period_ms = led_matrix_effective_profile_period_ms(profile);
    uint32_t sparkle_period_ms =
        MAX((uint32_t)2U, base_period_ms * STARLIGHT_SPARKLE_PERIOD_MULTIPLIER);
    uint32_t sparkle_window_ms =
        MAX((uint32_t)1U, sparkle_period_ms / STARLIGHT_SPARKLE_WINDOW_DIVISOR);

    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        uint32_t phase_ms =
            (now_ms + led_matrix_phase_offset_ms(led, sparkle_period_ms)) % sparkle_period_ms;
        uint8_t sparkle_pct =
            triangle_window_intensity(phase_ms, sparkle_window_ms, STARLIGHT_SPARKLE_MAX_PCT);
        struct led_rgb8 color = resolve_profile_color(profile, led, now_ms);

        color = scale_color(color, profile->brightness);
        logical_leds[led] = scale_color(color, sparkle_pct);
    }

    if (grid_cell_count == 0U || grid_row_count == 0U || grid_col_count == 0U) {
        return;
    }

    uint16_t valid_rows[LED_MATRIX_LOCAL_ROWS];
    bool row_has_led[LED_MATRIX_LOCAL_ROWS] = {0};
    uint16_t valid_row_count = 0U;

    for (size_t cell = 0; cell < grid_cell_count; cell++) {
        uint16_t led = cell_to_led[cell];
        uint16_t row = cell_row(cell);

        if (led == LM_NOLED || row >= grid_row_count || row_has_led[row]) {
            continue;
        }

        row_has_led[row] = true;
        valid_rows[valid_row_count++] = row;
    }

    if (valid_row_count == 0U) {
        return;
    }

    uint32_t meteor_period_ms = MAX((uint32_t)STARLIGHT_METEOR_MIN_PERIOD_MS,
                                    base_period_ms * STARLIGHT_METEOR_PERIOD_MULTIPLIER);
    uint32_t meteor_slot = now_ms / meteor_period_ms;
    uint32_t meteor_hash = led_matrix_hash32(meteor_slot + 0x51ED270BU);

    if ((meteor_hash % STARLIGHT_METEOR_ACTIVE_MODULUS) != 0U) {
        return;
    }

    bool reverse = (meteor_hash & BIT(0)) != 0U;
    uint16_t selected_row = valid_rows[(meteor_hash >> 1) % valid_row_count];
    uint32_t meteor_phase_ms = now_ms % meteor_period_ms;
    uint32_t travel_cols = grid_col_count + STARLIGHT_METEOR_TAIL_CELLS;
    int32_t head_col = (int32_t)(((uint64_t)meteor_phase_ms * travel_cols) / meteor_period_ms) -
                       (int32_t)STARLIGHT_METEOR_TAIL_CELLS;

    for (size_t cell = 0; cell < grid_cell_count; cell++) {
        uint16_t led = cell_to_led[cell];

        if (led == LM_NOLED || cell_row(cell) != selected_row) {
            continue;
        }

        uint32_t projected_col =
            reverse ? ((grid_col_count - 1U) - cell_col(cell)) : cell_col(cell);
        int32_t tail_delta = head_col - (int32_t)projected_col;
        uint8_t meteor_pct = 0U;

        switch (tail_delta) {
        case 0:
            meteor_pct = 100U;
            break;
        case 1:
            meteor_pct = STARLIGHT_METEOR_TAIL_NEAR_INTENSITY;
            break;
        case 2:
            meteor_pct = STARLIGHT_METEOR_TAIL_MID_INTENSITY;
            break;
        case 3:
            meteor_pct = STARLIGHT_METEOR_TAIL_FAR_INTENSITY;
            break;
        case 4:
            meteor_pct = STARLIGHT_METEOR_TAIL_END_INTENSITY;
            break;
        default:
            break;
        }

        if (meteor_pct == 0U) {
            continue;
        }

        struct led_rgb8 color = resolve_profile_color(profile, led, now_ms);
        color = scale_color(color, profile->brightness);
        logical_leds[led] = max_blend(logical_leds[led], scale_color(color, meteor_pct));
    }
}

static void
render_independent_profile_solid(const struct led_matrix_independent_group_cfg *group,
                                 const struct led_matrix_independent_profile_cfg *profile,
                                 uint32_t now_ms) {
    for (size_t i = 0; i < group->led_count; i++) {
        uint16_t led = group->leds[i];
        struct led_rgb8 color = resolve_independent_profile_color(profile, i, now_ms);
        logical_leds[led] = scale_color(color, profile->brightness);
    }
}

static void
render_independent_profile_breathe(const struct led_matrix_independent_group_cfg *group,
                                   const struct led_matrix_independent_profile_cfg *profile,
                                   uint32_t now_ms) {
    uint8_t intensity =
        breathe_intensity(led_matrix_effective_independent_profile_period_ms(profile), now_ms);

    for (size_t i = 0; i < group->led_count; i++) {
        uint16_t led = group->leds[i];
        struct led_rgb8 color = resolve_independent_profile_color(profile, i, now_ms);
        color = scale_color(color, profile->brightness);
        logical_leds[led] = scale_color(color, intensity);
    }
}

static void
render_independent_profile_spectrum(const struct led_matrix_independent_group_cfg *group,
                                    const struct led_matrix_independent_profile_cfg *profile,
                                    uint32_t now_ms) {
    uint16_t hue_offset =
        spectrum_hue_offset(led_matrix_effective_independent_profile_period_ms(profile), now_ms);

    for (size_t i = 0; i < group->led_count; i++) {
        uint16_t led = group->leds[i];
        struct led_rgb8 color =
            shift_color_hue(resolve_independent_profile_color(profile, i, now_ms), hue_offset);
        logical_leds[led] = scale_color(color, profile->brightness);
    }
}

static void
render_independent_profile_swirl(const struct led_matrix_independent_group_cfg *group,
                                 const struct led_matrix_independent_profile_cfg *profile,
                                 uint32_t now_ms) {
    uint16_t base_offset =
        spectrum_hue_offset(led_matrix_effective_independent_profile_period_ms(profile), now_ms);

    for (size_t i = 0; i < group->led_count; i++) {
        uint16_t led = group->leds[i];
        uint16_t led_offset =
            group->led_count > 0U ? (uint16_t)((SPECTRUM_HUE_MAX * i) / group->led_count) : 0U;
        struct led_rgb8 color =
            shift_color_hue(resolve_independent_profile_color(profile, i, now_ms),
                            (base_offset + led_offset) % 360U);
        logical_leds[led] = scale_color(color, profile->brightness);
    }
}

static void render_independent_group(const struct led_matrix_independent_group_cfg *group,
                                     uint32_t now_ms) {
    const struct led_matrix_independent_profile_cfg *profile =
        selected_independent_profile_cfg(group);

    if (group == NULL || profile == NULL) {
        return;
    }

    switch (profile->renderer) {
    case LED_MATRIX_RENDERER_SOLID:
        render_independent_profile_solid(group, profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_BREATHE:
        render_independent_profile_breathe(group, profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_SPECTRUM:
        render_independent_profile_spectrum(group, profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_SWIRL:
        render_independent_profile_swirl(group, profile, now_ms);
        break;
    default:
        break;
    }
}

static void render_independent_groups(uint32_t now_ms) {
    for (size_t i = 0; i < INDEPENDENT_GROUP_COUNT; i++) {
        render_independent_group(&independent_groups[i], now_ms);
    }
}

static void render_profile(const struct led_matrix_profile_cfg *profile, uint32_t now_ms) {
    switch (profile->renderer) {
    case LED_MATRIX_RENDERER_SOLID:
        render_profile_solid(profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_BREATHE:
        render_profile_breathe(profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_SPECTRUM:
        render_profile_spectrum(profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_SWIRL:
        render_profile_swirl(profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_KEYPRESS:
        render_profile_keypress(profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_RIPPLE:
        render_profile_ripple(profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_RAIN:
        render_profile_rain(profile, now_ms);
        break;
    case LED_MATRIX_RENDERER_STARLIGHT:
        render_profile_starlight(profile, now_ms);
        break;
    default:
        break;
    }
}

static void render_layer_color(uint32_t now_ms) {
    if (!layer_color.active || layer_color.cfg == NULL) {
        return;
    }

    const struct led_matrix_layer_color_cfg *cfg = layer_color.cfg;
    struct led_zone zone = led_matrix_layer_color_zone_for_cfg(cfg);

    for (size_t i = 0; i < zone.count; i++) {
        uint16_t led = zone.leds[i];
        struct led_rgb8 color =
            resolve_zone_color(cfg->layer_colors, cfg->layer_color_count, 0U, i, now_ms);
        logical_leds[led] = scale_color(color, cfg->brightness);
    }
}

static void render_battery_overlay(uint32_t now_ms) {
    if (battery_zone.count == 0U) {
        return;
    }

    uint8_t soc = MIN(snapshot.battery_soc, (uint8_t)100);
    size_t lit = (size_t)(((uint32_t)soc * battery_zone.count + 99U) / 100U);

    for (size_t i = 0; i < battery_zone.count; i++) {
        logical_leds[battery_zone.leds[i]] =
            i < lit ? resolve_battery_on_color(soc, i, now_ms)
                    : resolve_zone_color(battery_color_off_cfg.colors, battery_color_off_cfg.count,
                                         battery_color_off_cfg.default_color, i, now_ms);
    }
}

static void render_usb_overlay(uint32_t now_ms) {
    if (usb_zone.count == 0U) {
        return;
    }

    const struct led_matrix_color_list_cfg *colors =
        led_matrix_usb_color_cfg_for_state(snapshot.usb_state);
    set_zone_color_from_config(&usb_zone, colors->colors, colors->count, colors->default_color,
                               now_ms);
}

static void render_ble_overlay(uint32_t now_ms) {
    if (ble_zone.count == 0U) {
        return;
    }

    for (size_t i = 0; i < ble_zone.count; i++) {
        logical_leds[ble_zone.leds[i]] = (struct led_rgb8){0, 0, 0};
    }

    if (snapshot.ble_profile >= ble_zone.count) {
        return;
    }

    const struct led_matrix_color_list_cfg *colors =
        led_matrix_ble_color_cfg_for_snapshot(&snapshot);
    struct led_rgb8 color = resolve_zone_color(colors->colors, colors->count, colors->default_color,
                                               snapshot.ble_profile, now_ms);

    if (ble_pairing_transient_active) {
        color = scale_color(color, blink_intensity(600U, now_ms));
    }

    logical_leds[ble_zone.leds[snapshot.ble_profile]] = color;
}

static void render_hid_overlay(uint32_t now_ms) {
    /* Shared HID LEDs use this fixed write order; later indicators win without blending. */
    for (size_t i = 0; i < ARRAY_SIZE(hid_indicator_descriptors); i++) {
        render_hid_indicator(&hid_indicator_descriptors[i], now_ms);
    }
}

static void render_status_overlay(uint32_t now_ms) {
    uint8_t sources = current_status_sources();

    /* Shared status LEDs use this fixed overwrite order; later zones win without blending. */
    for (size_t i = 0; i < ARRAY_SIZE(status_source_descriptors); i++) {
        const struct led_matrix_status_source_descriptor *cfg = &status_source_descriptors[i];

        if ((sources & cfg->source) == 0U) {
            continue;
        }

        cfg->render_overlay(now_ms);
    }
}

static void render_active_base_source(uint32_t now_ms) {
    const struct led_matrix_profile_cfg *profile = selected_profile_cfg();

    if (!led_matrix_base_render_enabled()) {
        return;
    }

    if (profile != NULL) {
        render_profile(profile, now_ms);
    }

    if (led_matrix_has_independent_groups()) {
        render_independent_groups(now_ms);
    }

    if (layer_color.active && layer_color.cfg != NULL) {
        render_layer_color(now_ms);
    }
}

static void led_matrix_invalidate_flushed_frame(void) { last_flushed_frame_valid = false; }

static bool led_matrix_frame_matches_last_flush(void) {
    if (!last_flushed_frame_valid ||
        memcmp(last_flushed_logical_leds, logical_leds, sizeof(logical_leds)) != 0) {
        return false;
    }

    if (last_flushed_brightness == state.brightness) {
        return true;
    }

    for (size_t i = 0; i < ARRAY_SIZE(logical_leds); i++) {
        if (logical_leds[i].r != 0U || logical_leds[i].g != 0U || logical_leds[i].b != 0U) {
            return false;
        }
    }

    return true;
}

static void led_matrix_store_flushed_frame(void) {
    memcpy(last_flushed_logical_leds, logical_leds, sizeof(logical_leds));
    last_flushed_brightness = state.brightness;
    last_flushed_frame_valid = true;
}

static int led_matrix_flush_cleared_output(void) {
    clear_logical_leds();

    if (led_matrix_frame_matches_last_flush()) {
        return 0;
    }

    int err = flush_matrix();
    if (err == 0) {
        led_matrix_store_flushed_frame();
    } else {
        led_matrix_invalidate_flushed_frame();
    }

    return err;
}

#if LED_DRIVER_IS_MATRIX
static int flush_matrix_channel_sink(void) {
    int err = led_write_channels(output_dev, 0, OUTPUT_CHANNEL_COUNT, channel_values);
    if (err != 0) {
        LOG_ERR("Failed writing LED matrix channels (%d)", err);
    }

    return err;
}

static int flush_matrix_rgb_sink(void) {
    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        const struct led_matrix_rgb_sink_entry *entry = &matrix_output_sink.rgb_leds[led];
        struct led_rgb8 color = scale_color(logical_leds[led], state.brightness);
        uint8_t ordered[RGB_CHANNEL_COUNT_PER_LED];

        ordered[entry->red_slot] = color.r;
        ordered[entry->green_slot] = color.g;
        ordered[entry->blue_slot] = color.b;

        int err = led_set_color(output_dev, entry->led, RGB_CHANNEL_COUNT_PER_LED, ordered);
        if (err != 0) {
            LOG_ERR("Failed setting LED matrix color at %u (%d)", entry->led, err);
            return err;
        }
    }

    return 0;
}

static int flush_matrix_scalar_led_sink(bool binary_only) {
    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        struct led_rgb8 color = scale_color(logical_leds[led], state.brightness);
        int err = write_scalar_led_value(matrix_output_sink.scalar_leds[led],
                                         grayscale_value(color), binary_only);
        if (err != 0) {
            LOG_ERR("Failed setting LED matrix scalar output at %u (%d)",
                    matrix_output_sink.scalar_leds[led], err);
            return err;
        }
    }

    return 0;
}

static int flush_matrix_scalar_triplet_sink(bool binary_only) {
    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        struct led_rgb8 color = scale_color(logical_leds[led], state.brightness);
        uint8_t raw_values[RGB_CHANNEL_COUNT_PER_LED] = {color.r, color.g, color.b};

        for (uint8_t channel = 0; channel < RGB_CHANNEL_COUNT_PER_LED; channel++) {
            uint16_t logical_channel = (led * RGB_CHANNEL_COUNT_PER_LED) + channel;
            int err = write_scalar_led_value(matrix_output_sink.scalar_channels[logical_channel],
                                             raw_values[channel], binary_only);
            if (err != 0) {
                LOG_ERR("Failed setting LED matrix triplet output at %u (%d)",
                        matrix_output_sink.scalar_channels[logical_channel], err);
                return err;
            }
        }
    }

    return 0;
}
#endif

#if LED_DRIVER_IS_STRIP
static int flush_strip_device(void) {
    memset(strip_pixels, 0, sizeof(strip_pixels));

    for (uint16_t channel = 0; channel < OUTPUT_CHANNEL_COUNT; channel++) {
        uint16_t pixel = channel / RGB_CHANNEL_COUNT_PER_LED;
        uint8_t channel_in_pixel = channel % RGB_CHANNEL_COUNT_PER_LED;
        uint8_t value = channel_values[channel];

        switch (channel_in_pixel) {
        case 0:
            strip_pixels[pixel].r = value;
            break;
        case 1:
            strip_pixels[pixel].g = value;
            break;
        case 2:
            strip_pixels[pixel].b = value;
            break;
        default:
            break;
        }
    }

    int err = led_strip_update_rgb(output_dev, strip_pixels, OUTPUT_PIXEL_COUNT);
    if (err != 0) {
        LOG_ERR("Failed updating LED strip pixels (%d)", err);
    }

    return err;
}
#endif

static int flush_matrix(void) {
#if LED_DRIVER_IS_MATRIX
    switch (matrix_output_sink.kind) {
    case LED_MATRIX_MATRIX_SINK_CHANNELS:
        memset(channel_values, 0, sizeof(channel_values));

        for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
            struct led_rgb8 color = scale_color(logical_leds[led], state.brightness);
            channel_values[led_to_channel(led, 0)] = color.r;
            channel_values[led_to_channel(led, 1)] = color.g;
            channel_values[led_to_channel(led, 2)] = color.b;
        }

        return flush_matrix_channel_sink();
    case LED_MATRIX_MATRIX_SINK_RGB_SET_COLOR:
        return flush_matrix_rgb_sink();
    case LED_MATRIX_MATRIX_SINK_SCALAR_LED:
        return flush_matrix_scalar_led_sink(false);
    case LED_MATRIX_MATRIX_SINK_SCALAR_TRIPLET:
        return flush_matrix_scalar_triplet_sink(false);
    case LED_MATRIX_MATRIX_SINK_BINARY_LED:
        return flush_matrix_scalar_led_sink(true);
    case LED_MATRIX_MATRIX_SINK_BINARY_TRIPLET:
        return flush_matrix_scalar_triplet_sink(true);
    case LED_MATRIX_MATRIX_SINK_NONE:
    default:
        return -ENOTSUP;
    }
#elif LED_DRIVER_IS_STRIP
    memset(channel_values, 0, sizeof(channel_values));

    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        struct led_rgb8 color = scale_color(logical_leds[led], state.brightness);
        channel_values[led_to_channel(led, 0)] = color.r;
        channel_values[led_to_channel(led, 1)] = color.g;
        channel_values[led_to_channel(led, 2)] = color.b;
    }

    return flush_strip_device();
#else
    return -ENOTSUP;
#endif
}

static int render_and_flush(void) {
    uint32_t now_ms = k_uptime_get_32();
    uint8_t status_sources = current_status_sources();

    clear_logical_leds();

    if (led_matrix_base_render_enabled()) {
        render_active_base_source(now_ms);

        if (status_sources != 0U) {
            render_status_overlay(now_ms);
        }
    } else if (force_show_active && status_sources != 0U) {
        render_status_overlay(now_ms);
    }

    if (led_matrix_frame_matches_last_flush()) {
        return 0;
    }

    int err = flush_matrix();
    if (err == 0) {
        led_matrix_store_flushed_frame();

        if (runtime_first_frame_after_wake_pending) {
            LOG_DBG("LED matrix first render after wake: source=%d runtime=%s",
                    led_matrix_effective_source(),
                    led_matrix_runtime_mode_name(led_matrix_current_runtime_mode()));
            runtime_first_frame_after_wake_pending = false;
        }
    } else {
        led_matrix_invalidate_flushed_frame();
    }

    return err;
}

static uint16_t timeout_for_source(enum led_matrix_status_source src) {
    const struct led_matrix_status_source_descriptor *cfg =
        led_matrix_status_source_descriptor_for_source(src);
    return cfg != NULL ? cfg->timeout_ms : 0U;
}

static uint32_t
timeout_for_sources_with_wake_context(uint8_t sources,
                                      enum led_matrix_status_wake_context wake_context) {
    uint32_t timeout_ms = 0U;

    for (size_t i = 0; i < ARRAY_SIZE(status_source_descriptors); i++) {
        const struct led_matrix_status_source_descriptor *cfg = &status_source_descriptors[i];

        if ((sources & cfg->source) == 0U) {
            continue;
        }

        uint32_t source_timeout = cfg->timeout_ms;
        if (led_matrix_wake_context_requires_transient_status(wake_context) &&
            source_timeout == 0U) {
            source_timeout = LED_MATRIX_STATUS_ONLY_PERSISTENT_TIMEOUT_MS;
        }

        timeout_ms = MAX(timeout_ms, source_timeout);
    }

    return timeout_ms;
}

static void led_matrix_queue_service_work(uint32_t requests) {
    int ret;

    atomic_or(&led_matrix_service_requests, (atomic_val_t)requests);
    ret = k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &led_matrix_render_work);
    if (ret < 0) {
        LOG_ERR("Failed to submit LED matrix service work (%d)", ret);
    }
}

static int led_matrix_reschedule_delayable_work(struct k_work_delayable *work, k_timeout_t delay) {
    return k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), work, delay);
}

static void led_matrix_sync_tick_state(void) {
    bool should_run =
        led_matrix_render_enabled() && led_matrix_needs_periodic_refresh(k_uptime_get_32());

    if (should_run == led_matrix_tick_running) {
        return;
    }

    if (should_run) {
        k_timer_start(&led_matrix_tick, K_MSEC(LED_MATRIX_TICK_MS), K_MSEC(LED_MATRIX_TICK_MS));
    } else {
        k_timer_stop(&led_matrix_tick);
    }

    led_matrix_tick_running = should_run;
    LOG_DBG("LED matrix periodic refresh %s", should_run ? "started" : "stopped");
}

static void led_matrix_render_work_handler(struct k_work *work) {
    uint32_t requests;
    int err = 0;

    ARG_UNUSED(work);
    requests = (uint32_t)atomic_clear(&led_matrix_service_requests);

    if ((requests & LED_MATRIX_SERVICE_SYNC_OUTPUT) != 0U) {
        if (led_matrix_render_enabled()) {
            err = led_matrix_sync_active_runtime_output();
        } else {
            err = led_matrix_clear_and_disable_runtime_output();
        }

        if (err != 0) {
            LOG_ERR("LED matrix output sync failed (%d)", err);
        }
    }

    if (err == 0 && (requests & LED_MATRIX_SERVICE_RENDER) != 0U && led_matrix_render_enabled()) {
        err = render_and_flush();
        if (err != 0) {
            LOG_ERR("LED matrix render failed (%d)", err);
        }
    }

    led_matrix_sync_tick_state();

    if (atomic_get(&led_matrix_service_requests) != 0U) {
        (void)k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &led_matrix_render_work);
    }
}

K_WORK_DEFINE(led_matrix_render_work, led_matrix_render_work_handler);

static void led_matrix_schedule_render(void) {
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_RENDER);
}

static void led_matrix_tick_handler(struct k_timer *timer) {
    ARG_UNUSED(timer);

    if (!led_matrix_render_enabled()) {
        return;
    }

    led_matrix_schedule_render();
}

K_TIMER_DEFINE(led_matrix_tick, led_matrix_tick_handler, NULL);
