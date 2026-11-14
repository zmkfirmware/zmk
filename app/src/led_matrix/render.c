/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "internal.h"

static int flush_matrix(void);

#include "render_demand.c"

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

    led_matrix_mark_independent_time(group, profile);

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
    led_matrix_mark_profile_time(profile, now_ms);
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
        logical_leds_dynamic[led] = led_matrix_color_needs_time(
            led_matrix_configured_color(cfg->layer_colors, cfg->layer_color_count, LM_BLACK, i),
            cfg->brightness, false);
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
        uint32_t color;
        if (i < lit && battery_level_color_cfg.count > 0U) {
            size_t bucket = MIN(battery_level_color_cfg.count - 1U,
                                ((size_t)soc * battery_level_color_cfg.count) / 101U);
            color = led_matrix_configured_color(battery_level_color_cfg.colors,
                                                battery_level_color_cfg.count,
                                                battery_level_color_cfg.default_color, bucket);
        } else {
            const struct led_matrix_color_list_cfg *cfg =
                i < lit ? &battery_color_on_cfg : &battery_color_off_cfg;
            color = led_matrix_configured_color(cfg->colors, cfg->count, cfg->default_color, i);
        }
        logical_leds_dynamic[battery_zone.leds[i]] =
            led_matrix_color_needs_time(color, 100U, false);
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
        logical_leds_dynamic[ble_zone.leds[i]] = false;
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
    logical_leds_dynamic[ble_zone.leds[snapshot.ble_profile]] = led_matrix_color_needs_time(
        led_matrix_configured_color(colors->colors, colors->count, colors->default_color,
                                    snapshot.ble_profile),
        100U, ble_pairing_transient_active);
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
    int first_err = 0;
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
            if (first_err == 0) {
                first_err = err;
            }
        }
    }

    return first_err;
}

static int flush_matrix_scalar_led_sink(bool binary_only) {
    int first_err = 0;
    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        struct led_rgb8 color = scale_color(logical_leds[led], state.brightness);
        int err = write_scalar_led_value(matrix_output_sink.scalar_leds[led],
                                         grayscale_value(color), binary_only);
        if (err != 0) {
            LOG_ERR("Failed setting LED matrix scalar output at %u (%d)",
                    matrix_output_sink.scalar_leds[led], err);
            if (first_err == 0) {
                first_err = err;
            }
        }
    }

    return first_err;
}

static int flush_matrix_scalar_triplet_sink(bool binary_only) {
    int first_err = 0;
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
                if (first_err == 0) {
                    first_err = err;
                }
            }
        }
    }

    return first_err;
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

static struct led_matrix_render_demand compose_render_frame(uint32_t now_ms) {
    uint8_t status_sources = current_status_sources();
    struct led_matrix_render_demand demand = {0};

    clear_logical_leds();

    if (led_matrix_base_render_enabled()) {
        render_active_base_source(now_ms);

        if (status_sources != 0U) {
            render_status_overlay(now_ms);
        }
    } else if (force_show_active && status_sources != 0U) {
        render_status_overlay(now_ms);
    }

    for (size_t led = 0; led < ARRAY_SIZE(logical_leds); led++) {
        struct led_rgb8 color = scale_color(logical_leds[led], state.brightness);
        bool nonzero = color.r != 0U || color.g != 0U || color.b != 0U;
#if LED_DRIVER_IS_MATRIX
        if (matrix_output_sink.kind == LED_MATRIX_MATRIX_SINK_SCALAR_LED ||
            matrix_output_sink.kind == LED_MATRIX_MATRIX_SINK_SCALAR_TRIPLET) {
            nonzero = ((uint16_t)grayscale_value(color) * 100U) / 255U != 0U;
        }
#endif
        demand.nonzero |= nonzero;
        demand.periodic |= logical_leds_dynamic[led];
    }
    return demand;
}

static int flush_render_frame(void) {
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

    if ((requests & LED_MATRIX_SERVICE_SYNC_OUTPUT) != 0U) {
        /* An old delayed retry must not apply an earlier on/off request. */
        (void)atomic_inc(&led_matrix_output_generation);
    }
    atomic_or(&led_matrix_service_requests, (atomic_val_t)requests);
    ret = k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &led_matrix_render_work);
    if (ret < 0) {
        LOG_ERR("Failed to submit LED matrix service work (%d)", ret);
    }
}

static int led_matrix_reschedule_delayable_work(struct k_work_delayable *work, k_timeout_t delay) {
    return k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), work, delay);
}

static struct k_spinlock led_matrix_tick_lock;

static void led_matrix_sync_tick_state(void) {
    /* Activity events can preempt the low-priority service. Keep the decision and
     * timer update together so an old start cannot undo a concurrent stop. */
    k_spinlock_key_t key = k_spin_lock(&led_matrix_tick_lock);
    bool should_run =
        !atomic_get(&led_matrix_output_sync_failed) && led_matrix_render_enabled() &&
        atomic_get(&led_matrix_demand_generation) == atomic_get(&led_matrix_output_generation) &&
        current_render_demand.periodic;

    if (should_run == led_matrix_tick_running) {
        k_spin_unlock(&led_matrix_tick_lock, key);
        return;
    }

    if (should_run) {
        k_timer_start(&led_matrix_tick, K_MSEC(LED_MATRIX_TICK_MS), K_MSEC(LED_MATRIX_TICK_MS));
    } else {
        k_timer_stop(&led_matrix_tick);
    }

    led_matrix_tick_running = should_run;
    k_spin_unlock(&led_matrix_tick_lock, key);
    LOG_DBG("LED matrix periodic refresh %s", should_run ? "started" : "stopped");
}

/* Only the output service accesses retry bookkeeping; the callback just queues a request. */
static const uint16_t led_matrix_output_retry_delays_ms[] = {100, 500, 2000};
static uint8_t led_matrix_output_retry_count;
static atomic_val_t led_matrix_output_retry_generation;

static void led_matrix_output_retry_handler(struct k_work *work) {
    ARG_UNUSED(work);
    if (!led_matrix_is_sleep_suspended()) {
        led_matrix_queue_service_work(LED_MATRIX_SERVICE_RETRY_OUTPUT | LED_MATRIX_SERVICE_RENDER);
    }
}

K_WORK_DELAYABLE_DEFINE(led_matrix_output_retry_work, led_matrix_output_retry_handler);

static void led_matrix_reset_output_retry(void) {
    (void)k_work_cancel_delayable(&led_matrix_output_retry_work);
    led_matrix_output_retry_count = 0U;
}

static void led_matrix_retry_output_sync(atomic_val_t generation) {
    if (led_matrix_is_sleep_suspended() ||
        generation != atomic_get(&led_matrix_output_generation)) {
        return;
    }
    if (led_matrix_output_retry_count >= ARRAY_SIZE(led_matrix_output_retry_delays_ms)) {
        LOG_ERR("LED matrix output sync retries exhausted; waiting for a new output request");
        return;
    }

    uint16_t delay_ms = led_matrix_output_retry_delays_ms[led_matrix_output_retry_count++];
    led_matrix_output_retry_generation = generation;
    int err = led_matrix_reschedule_delayable_work(&led_matrix_output_retry_work, K_MSEC(delay_ms));
    if (err < 0) {
        LOG_ERR("LED matrix output retry scheduling failed (%d)", err);
    } else {
        LOG_DBG("LED matrix output retry %u scheduled in %u ms", led_matrix_output_retry_count,
                delay_ms);
    }
}

static void led_matrix_render_work_handler(struct k_work *work) {
    uint32_t requests;
    int err = 0;

    ARG_UNUSED(work);
    requests = (uint32_t)atomic_clear(&led_matrix_service_requests);
    atomic_val_t generation = atomic_get(&led_matrix_output_generation);
    bool sync_output = (requests & LED_MATRIX_SERVICE_SYNC_OUTPUT) != 0U;

    if (sync_output) {
        led_matrix_reset_output_retry();
    } else if ((requests & LED_MATRIX_SERVICE_RETRY_OUTPUT) != 0U) {
        sync_output = atomic_get(&led_matrix_output_sync_failed) &&
                      generation == led_matrix_output_retry_generation &&
                      !led_matrix_is_sleep_suspended();
        if (!sync_output) {
            requests &= ~LED_MATRIX_SERVICE_RENDER;
        }
    }

    if (sync_output || (!atomic_get(&led_matrix_output_sync_failed) &&
                        (requests & LED_MATRIX_SERVICE_RENDER) != 0U)) {
        struct led_matrix_render_demand demand = compose_render_frame(k_uptime_get_32());
        if (generation != atomic_get(&led_matrix_output_generation)) {
            goto finished;
        }

        current_render_demand = demand;
        atomic_set(&led_matrix_demand_generation, generation);
        bool power_required = led_matrix_render_enabled() && (demand.nonzero || demand.periodic);
        err = power_required ? led_matrix_sync_active_runtime_output()
                             : led_matrix_clear_and_disable_runtime_output();

        if (generation != atomic_get(&led_matrix_output_generation)) {
            led_matrix_invalidate_flushed_frame();
            goto finished;
        }

        if (err == 0 && power_required && led_matrix_render_enabled()) {
            err = flush_render_frame();
            if (err != 0) {
                /* Do not leave a newly awakened, unusable output powered while retrying. */
                atomic_set(&led_matrix_output_sync_failed, true);
                int off_err = led_matrix_clear_and_disable_runtime_output();
                if (off_err != 0) {
                    LOG_WRN("LED matrix failed render rollback (%d)", off_err);
                }
            }
        }

        if (generation != atomic_get(&led_matrix_output_generation)) {
            led_matrix_invalidate_flushed_frame();
            goto finished;
        }
        atomic_set(&led_matrix_output_sync_failed, err != 0);
        if (err != 0) {
            LOG_ERR("LED matrix output sync failed (%d)", err);
            led_matrix_retry_output_sync(generation);
        } else {
            led_matrix_reset_output_retry();
        }
    }

finished:
    led_matrix_sync_tick_state();

    if (atomic_get(&led_matrix_service_requests) != 0U) {
        (void)k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &led_matrix_render_work);
    }
}

K_WORK_DEFINE(led_matrix_render_work, led_matrix_render_work_handler);

static void led_matrix_schedule_render(void) {
    /* Events can change the target even while an old output request is retrying. */
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER);
}

static void led_matrix_tick_handler(struct k_timer *timer) {
    ARG_UNUSED(timer);

    if (!led_matrix_render_enabled()) {
        return;
    }

    /* A periodic frame must not invalidate pending retries or reset their budget. */
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_RENDER);
}

K_TIMER_DEFINE(led_matrix_tick, led_matrix_tick_handler, NULL);
