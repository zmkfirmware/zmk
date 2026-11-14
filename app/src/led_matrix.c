/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3733.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <drivers/ext_power.h>
#include <drivers/led/is31_power.h>
#include <drivers/led/is31fl3737.h>
#include <drivers/led/led_composite.h>

#include <dt-bindings/zmk/led_matrix.h>
#include <dt-bindings/zmk/led_matrix_colors.h>

#include <zmk/activity.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/led_matrix_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/hid_indicators.h>
#include <zmk/keymap.h>
#include <zmk/led_matrix.h>
#include <zmk/matrix.h>
#include <zmk/matrix_transform.h>
#include <zmk/usb.h>
#include <zmk/workqueue.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_CHOSEN(zmk_led_matrix)
#error "CONFIG_ZMK_LED_MATRIX is enabled but no zmk,led-matrix chosen node found"
#endif

#define MATRIX_NODE DT_CHOSEN(zmk_led_matrix)
#define LOGICAL_LED_COUNT DT_PROP(MATRIX_NODE, led_count)
#define RGB_CHANNEL_COUNT_PER_LED 3
#define LOGICAL_CHANNEL_COUNT (LOGICAL_LED_COUNT * RGB_CHANNEL_COUNT_PER_LED)
#define OUTPUT_CHANNEL_COUNT DT_PROP_OR(MATRIX_NODE, output_channel_count, LOGICAL_CHANNEL_COUNT)

#if !DT_NODE_HAS_PROP(MATRIX_NODE, led_driver)
#error "zmk,led-matrix node must define led-driver"
#endif

#define LED_MATRIX_DRIVER_MATRIX 0
#define LED_MATRIX_DRIVER_STRIP 1

#define LED_DRIVER_BACKEND DT_ENUM_IDX(MATRIX_NODE, led_driver)
#define LED_DRIVER_IS_MATRIX (LED_DRIVER_BACKEND == LED_MATRIX_DRIVER_MATRIX)
#define LED_DRIVER_IS_STRIP (LED_DRIVER_BACKEND == LED_MATRIX_DRIVER_STRIP)
#define LED_MATRIX_HAS_CENTRAL_STATE                                                               \
    (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

BUILD_ASSERT(LOGICAL_LED_COUNT > 0, "zmk,led-matrix led-count must be > 0");
BUILD_ASSERT(LED_DRIVER_BACKEND == LED_MATRIX_DRIVER_MATRIX ||
                 LED_DRIVER_BACKEND == LED_MATRIX_DRIVER_STRIP,
             "zmk,led-matrix led-driver must be matrix or strip");
BUILD_ASSERT(LOGICAL_LED_COUNT < LM_NOLED,
             "zmk,led-matrix led-count must be smaller than LM_NOLED sentinel");
BUILD_ASSERT(OUTPUT_CHANNEL_COUNT > 0, "zmk,led-matrix output-channel-count must be > 0");

#if LED_DRIVER_IS_MATRIX
#if !DT_NODE_HAS_PROP(MATRIX_NODE, led_device)
#error "zmk,led-matrix with led-driver=matrix requires led-device"
#endif
#define MATRIX_OUTPUT_DEVICE_NODE DT_PHANDLE(MATRIX_NODE, led_device)
static const struct device *const output_dev = DEVICE_DT_GET(DT_PHANDLE(MATRIX_NODE, led_device));
#define OUTPUT_DEVICE_LED_COUNT DT_CHILD_NUM_STATUS_OKAY(MATRIX_OUTPUT_DEVICE_NODE)
#define LED_MATRIX_BACKEND_IS31FL3733 DT_NODE_HAS_COMPAT(MATRIX_OUTPUT_DEVICE_NODE, issi_is31fl3733)
#define LED_MATRIX_BACKEND_IS31FL3737 DT_NODE_HAS_COMPAT(MATRIX_OUTPUT_DEVICE_NODE, zmk_is31fl3737)
#define LED_MATRIX_BACKEND_IS_COMPOSITE                                                            \
    DT_NODE_HAS_COMPAT(MATRIX_OUTPUT_DEVICE_NODE, zmk_led_composite)
#define LED_MATRIX_BACKEND_HAS_MANAGED_POWER                                                       \
    (LED_MATRIX_BACKEND_IS31FL3733 || LED_MATRIX_BACKEND_IS31FL3737 ||                             \
     LED_MATRIX_BACKEND_IS_COMPOSITE)
#if LED_MATRIX_BACKEND_IS31FL3733 || LED_MATRIX_BACKEND_IS31FL3737
static const struct gpio_dt_spec backend_sdb =
    GPIO_DT_SPEC_GET_OR(MATRIX_OUTPUT_DEVICE_NODE, sdb_gpios, {0});
static const struct i2c_dt_spec backend_i2c = I2C_DT_SPEC_GET(MATRIX_OUTPUT_DEVICE_NODE);
static const uint8_t backend_current_limit =
    DT_PROP_OR(MATRIX_OUTPUT_DEVICE_NODE, current_limit, 0xFF);
#if LED_MATRIX_BACKEND_IS31FL3733
static const uint8_t backend_sync = DT_ENUM_IDX_OR(MATRIX_OUTPUT_DEVICE_NODE, sync_mode, 0);
static const struct zmk_is31_power_config backend_is31_power = {
    .chip = ZMK_IS31_CHIP_IS31FL3733,
    .dev = output_dev,
    .bus = backend_i2c,
    .sdb = backend_sdb,
    .current_limit = backend_current_limit,
    .sync = backend_sync,
};
#else
static const struct zmk_is31_power_config backend_is31_power = {
    .chip = ZMK_IS31_CHIP_IS31FL3737,
    .dev = output_dev,
    .bus = backend_i2c,
    .sdb = backend_sdb,
    .current_limit = backend_current_limit,
    .sync = 0,
};
#endif
#endif
#elif LED_DRIVER_IS_STRIP
#if !DT_NODE_HAS_PROP(MATRIX_NODE, led_strip_device)
#error "zmk,led-matrix with led-driver=strip requires led-strip-device"
#endif
static const struct device *const output_dev =
    DEVICE_DT_GET(DT_PHANDLE(MATRIX_NODE, led_strip_device));

#define LED_MATRIX_BACKEND_IS31FL3733 0
#define LED_MATRIX_BACKEND_IS31FL3737 0
#define LED_MATRIX_BACKEND_IS_COMPOSITE 0
#define LED_MATRIX_BACKEND_HAS_MANAGED_POWER 0
#endif

#include "led_matrix/core.c"

#include "led_matrix/power.c"

#include "led_matrix/status.c"

static int validate_zone_indices(const char *name, const uint16_t *leds, size_t count,
                                 bool *seen_mask) {
    for (size_t i = 0; i < count; i++) {
        uint16_t led = leds[i];
        if (led >= LOGICAL_LED_COUNT) {
            LOG_ERR("%s index out of range: %u", name, led);
            return -EINVAL;
        }

        if (seen_mask != NULL && seen_mask[led]) {
            LOG_ERR("%s has duplicate/overlap LED index: %u", name, led);
            return -EINVAL;
        }

        if (seen_mask != NULL) {
            seen_mask[led] = true;
        }
    }

    return 0;
}

static int validate_unique_zone_indices(const char *name, const struct led_zone *zone) {
    bool seen[LOGICAL_LED_COUNT];

    if (zone->count == 0U) {
        return 0;
    }

    memset(seen, 0, sizeof(seen));
    return validate_zone_indices(name, zone->leds, zone->count, seen);
}

static int validate_color_list(const char *name, const uint32_t *colors, size_t color_count,
                               size_t expected_count) {
    if (expected_count == 0U || color_count == 0U) {
        return 0;
    }

    if (color_count != 1U && color_count != expected_count) {
        LOG_ERR("%s length must be 1 or %d, got %d", name, (int)expected_count, (int)color_count);
        return -EINVAL;
    }

    for (size_t i = 0; i < color_count; i++) {
        if (!led_matrix_is_valid_configured_color(colors[i])) {
            LOG_ERR("%s entry uses unsupported color at index %d: 0x%08x", name, (int)i,
                    (unsigned int)colors[i]);
            return -EINVAL;
        }
    }

    return 0;
}

static int validate_bucket_color_list(const char *name, const uint32_t *colors,
                                      size_t color_count) {
    if (color_count == 0U) {
        return 0;
    }

    for (size_t i = 0; i < color_count; i++) {
        if (!led_matrix_is_valid_configured_color(colors[i])) {
            LOG_ERR("%s entry uses unsupported color at index %d: 0x%08x", name, (int)i,
                    (unsigned int)colors[i]);
            return -EINVAL;
        }
    }

    return 0;
}

static int validate_status_zone_configs(void) {
    for (size_t i = 0; i < ARRAY_SIZE(status_zone_validation_descriptors); i++) {
        const struct led_matrix_zone_validation_descriptor *cfg =
            &status_zone_validation_descriptors[i];
        int err = validate_unique_zone_indices(cfg->name, cfg->zone);

        if (err != 0) {
            return err;
        }
    }

    return 0;
}

static int validate_status_color_configs(void) {
    for (size_t i = 0; i < ARRAY_SIZE(status_color_validation_descriptors); i++) {
        const struct led_matrix_color_validation_descriptor *cfg =
            &status_color_validation_descriptors[i];
        int err;

        if (cfg->bucket_colors) {
            err = validate_bucket_color_list(cfg->name, cfg->colors->colors, cfg->colors->count);
        } else {
            size_t expected_count = cfg->expected_zone != NULL ? cfg->expected_zone->count : 0U;
            err = validate_color_list(cfg->name, cfg->colors->colors, cfg->colors->count,
                                      expected_count);
        }

        if (err != 0) {
            return err;
        }
    }

    for (size_t i = 0; i < ARRAY_SIZE(hid_indicator_descriptors); i++) {
        const struct led_matrix_hid_indicator_descriptor *cfg = &hid_indicator_descriptors[i];
        int err = validate_color_list(cfg->color_on_name, cfg->color_on->colors,
                                      cfg->color_on->count, cfg->zone->count);

        if (err != 0) {
            return err;
        }

        err = validate_color_list(cfg->color_off_name, cfg->color_off->colors,
                                  cfg->color_off->count, cfg->zone->count);
        if (err != 0) {
            return err;
        }
    }

    return 0;
}

static void rebuild_default_layer_color_leds(void) {
    default_layer_color_led_count = 0U;

    for (size_t i = 0; i < GRID_CELL_STORAGE_COUNT; i++) {
        default_layer_color_leds[i] = LM_NOLED;
    }

    for (size_t cell = 0; cell < grid_cell_count; cell++) {
        uint16_t led = cell_to_led[cell];

        if (led == LM_NOLED || led >= LOGICAL_LED_COUNT) {
            continue;
        }

        default_layer_color_leds[default_layer_color_led_count++] = led;
    }
}

static void clear_spatial_layout(void) {
    for (size_t i = 0; i < LOGICAL_LED_COUNT; i++) {
        led_to_cell[i] = LM_NOLED;
    }

    for (size_t i = 0; i < ZMK_KEYMAP_LEN; i++) {
        position_to_led[i] = LM_NOLED;
        position_to_cell[i] = LM_NOLED;
    }

    for (size_t i = 0; i < GRID_CELL_STORAGE_COUNT; i++) {
        cell_to_led[i] = LM_NOLED;
        cell_row_idx[i] = 0U;
        cell_col_idx[i] = 0U;
    }

    grid_cell_count = 0U;
    grid_row_count = 0U;
    grid_col_count = 0U;
    rebuild_default_layer_color_leds();
}

#if HAS_MATRIX_LED_GRID
static int32_t led_matrix_row_col_to_position(uint16_t row, uint16_t col) {
    return zmk_matrix_transform_row_column_to_position(LED_MATRIX_TRANSFORM, row, col);
}
#endif

static int validate_spatial_layout(void) {
    clear_spatial_layout();

#if !HAS_MATRIX_LED_GRID
    return 0;
#else
    size_t cell_count = 0U;
    uint16_t row_idx = 0U;
    size_t current_row_cell_count = 0U;
    bool previous_was_row_break = true;

    for (size_t token_idx = 0; token_idx < MATRIX_GRID_TOKEN_COUNT; token_idx++) {
        uint16_t token = matrix_led_grid[token_idx];

        if (token == LM_BREAK) {
            if (previous_was_row_break) {
                LOG_ERR("matrix-led-grid has empty row around token index %d", (int)token_idx);
                return -EINVAL;
            }

            if (current_row_cell_count != LED_MATRIX_LOCAL_COLS) {
                LOG_ERR("matrix-led-grid row %u has %d cells, expected local col count %d", row_idx,
                        (int)current_row_cell_count, (int)LED_MATRIX_LOCAL_COLS);
                return -EINVAL;
            }

            previous_was_row_break = true;
            row_idx++;
            current_row_cell_count = 0U;
            continue;
        }

        previous_was_row_break = false;

        if (row_idx >= LED_MATRIX_LOCAL_ROWS) {
            LOG_ERR(
                "matrix-led-grid has more rows than local matrix: token %d starts row %u, max %d",
                (int)token_idx, row_idx, (int)LED_MATRIX_LOCAL_ROWS);
            return -EINVAL;
        }

        if (cell_count >= GRID_CELL_STORAGE_COUNT || cell_count >= LM_NOLED) {
            LOG_ERR("matrix-led-grid cell count overflow");
            return -EINVAL;
        }

        uint16_t cell = (uint16_t)cell_count;
        cell_row_idx[cell] = row_idx;
        cell_col_idx[cell] = (uint16_t)current_row_cell_count;
        cell_to_led[cell] = LM_NOLED;

        int32_t position =
            led_matrix_row_col_to_position(row_idx, (uint16_t)current_row_cell_count);

        if (token == LM_PH) {
            if (position >= 0) {
                LOG_ERR("matrix-led-grid LM_PH at local RC(%u,%u) overlaps key position %d",
                        row_idx, (unsigned int)current_row_cell_count, (int)position);
                return -EINVAL;
            }

            current_row_cell_count++;
            cell_count++;
            continue;
        }

        if (position < 0 || position >= ZMK_KEYMAP_LEN) {
            LOG_ERR("matrix-led-grid local RC(%u,%u) has no valid key position", row_idx,
                    (unsigned int)current_row_cell_count);
            return -EINVAL;
        }

        if (position_to_cell[position] != LM_NOLED) {
            LOG_ERR("matrix-led-grid local RC(%u,%u) maps duplicate key position %d", row_idx,
                    (unsigned int)current_row_cell_count, (int)position);
            return -EINVAL;
        }

        if (token == LM_NOLED) {
            position_to_led[position] = LM_NOLED;
            position_to_cell[position] = cell;
            current_row_cell_count++;
            cell_count++;
            continue;
        }

        if (token >= LOGICAL_LED_COUNT) {
            LOG_ERR("matrix-led-grid token %d has invalid LED %u", (int)token_idx, token);
            return -EINVAL;
        }

        if (led_to_cell[token] != LM_NOLED) {
            LOG_ERR("matrix-led-grid has duplicate LED index %u", token);
            return -EINVAL;
        }

        led_to_cell[token] = cell;
        cell_to_led[cell] = token;
        position_to_led[position] = token;
        position_to_cell[position] = cell;
        current_row_cell_count++;
        cell_count++;
    }

    if (previous_was_row_break) {
        LOG_ERR("matrix-led-grid must not end with LM_BREAK");
        return -EINVAL;
    }

    if (current_row_cell_count != LED_MATRIX_LOCAL_COLS) {
        LOG_ERR("matrix-led-grid row %u has %d cells, expected local col count %d", row_idx,
                (int)current_row_cell_count, (int)LED_MATRIX_LOCAL_COLS);
        return -EINVAL;
    }

    if ((row_idx + 1U) != LED_MATRIX_LOCAL_ROWS) {
        LOG_ERR("matrix-led-grid row count mismatch: got %u, expected local row count %d",
                row_idx + 1U, (int)LED_MATRIX_LOCAL_ROWS);
        return -EINVAL;
    }

    if (cell_count == 0U) {
        LOG_ERR("matrix-led-grid produced no cells");
        return -EINVAL;
    }

    grid_cell_count = cell_count;
    grid_row_count = LED_MATRIX_LOCAL_ROWS;
    grid_col_count = LED_MATRIX_LOCAL_COLS;
    rebuild_default_layer_color_leds();
    return 0;
#endif
}

static int validate_independent_groups(void) {
    bool seen_leds[LOGICAL_LED_COUNT];

    memset(seen_leds, 0, sizeof(seen_leds));

    for (size_t i = 0; i < INDEPENDENT_GROUP_COUNT; i++) {
        const struct led_matrix_independent_group_cfg *group = &independent_groups[i];

        if (group->independent_id >= INDEPENDENT_GROUP_COUNT) {
            LOG_ERR("Independent child id out of range: %u (max %d)", group->independent_id,
                    (int)INDEPENDENT_GROUP_COUNT - 1);
            return -EINVAL;
        }

        for (size_t j = i + 1; j < INDEPENDENT_GROUP_COUNT; j++) {
            if (group->independent_id == independent_groups[j].independent_id) {
                LOG_ERR("Duplicate independent child id: %u", group->independent_id);
                return -EINVAL;
            }
        }

        if (group->led_count == 0U) {
            LOG_ERR("Independent child %u has no LEDs", group->independent_id);
            return -EINVAL;
        }

        for (size_t led_idx = 0; led_idx < group->led_count; led_idx++) {
            uint16_t led = group->leds[led_idx];

            if (led >= LOGICAL_LED_COUNT) {
                LOG_ERR("Independent child %u LED index out of range: %u", group->independent_id,
                        led);
                return -EINVAL;
            }

            for (size_t prev_idx = 0; prev_idx < led_idx; prev_idx++) {
                if (group->leds[prev_idx] == led) {
                    LOG_ERR("Independent child %u has duplicate LED index: %u",
                            group->independent_id, led);
                    return -EINVAL;
                }
            }

            if (seen_leds[led]) {
                LOG_ERR("Independent child %u overlaps another independent child on LED %u",
                        group->independent_id, led);
                return -EINVAL;
            }
            seen_leds[led] = true;

            if (led_matrix_has_spatial_grid() && led_to_cell[led] != LM_NOLED) {
                LOG_ERR("Independent child %u LED %u is already mapped in matrix-led-grid",
                        group->independent_id, led);
                return -EINVAL;
            }
        }

        if (group->profile_count == 0U) {
            LOG_ERR("Independent child %u requires at least one local profile",
                    group->independent_id);
            return -EINVAL;
        }

        for (size_t profile_idx = 0; profile_idx < group->profile_count; profile_idx++) {
            const struct led_matrix_independent_profile_cfg *profile =
                &group->profiles[profile_idx];

            if (profile->profile_id >= group->profile_count) {
                LOG_ERR("Independent child %u local profile id out of range: %u (max %d)",
                        group->independent_id, profile->profile_id, (int)group->profile_count - 1);
                return -EINVAL;
            }

            for (size_t other_idx = profile_idx + 1; other_idx < group->profile_count;
                 other_idx++) {
                if (profile->profile_id == group->profiles[other_idx].profile_id) {
                    LOG_ERR("Independent child %u duplicate local profile id: %u",
                            group->independent_id, profile->profile_id);
                    return -EINVAL;
                }
            }

            if (profile->renderer != LED_MATRIX_RENDERER_SOLID &&
                profile->renderer != LED_MATRIX_RENDERER_BREATHE &&
                profile->renderer != LED_MATRIX_RENDERER_SPECTRUM &&
                profile->renderer != LED_MATRIX_RENDERER_SWIRL) {
                LOG_ERR("Independent child %u local profile %u renderer %s is unsupported",
                        group->independent_id, profile->profile_id,
                        renderer_name(profile->renderer));
                return -EINVAL;
            }

            if (profile->brightness > 100U) {
                LOG_ERR("Independent child %u local profile %u brightness out of range: %u",
                        group->independent_id, profile->profile_id, profile->brightness);
                return -EINVAL;
            }

            if (profile->color_map_count != 1U && profile->color_map_count != group->led_count) {
                LOG_ERR(
                    "Independent child %u local profile %u colors length must be 1 or %d, got %d",
                    group->independent_id, profile->profile_id, (int)group->led_count,
                    (int)profile->color_map_count);
                return -EINVAL;
            }

            for (size_t color_idx = 0; color_idx < profile->color_map_count; color_idx++) {
                if (!led_matrix_is_valid_configured_color(profile->color_map[color_idx])) {
                    LOG_ERR("Independent child %u local profile %u uses unsupported color at %d: "
                            "0x%08x",
                            group->independent_id, profile->profile_id, (int)color_idx,
                            (unsigned int)profile->color_map[color_idx]);
                    return -EINVAL;
                }
            }
        }
    }

    for (size_t id = 0; id < INDEPENDENT_GROUP_COUNT; id++) {
        if (find_independent_group(id) == NULL) {
            LOG_ERR("Missing independent child id %d", (int)id);
            return -EINVAL;
        }
    }

    for (size_t group_idx = 0; group_idx < INDEPENDENT_GROUP_COUNT; group_idx++) {
        const struct led_matrix_independent_group_cfg *group = &independent_groups[group_idx];

        for (size_t profile_id = 0; profile_id < group->profile_count; profile_id++) {
            if (find_independent_profile(group, profile_id) == NULL) {
                LOG_ERR("Independent child %u missing local profile id %d", group->independent_id,
                        (int)profile_id);
                return -EINVAL;
            }
        }
    }

    return 0;
}

static int validate_profiles(void) {
    if (!led_matrix_has_profiles()) {
        return 0;
    }

    bool seen_profile_ids[MAX(1, DISPLAY_PROFILE_COUNT)];
    memset(seen_profile_ids, 0, sizeof(seen_profile_ids));

    for (size_t i = 0; i < DISPLAY_PROFILE_COUNT; i++) {
        const struct led_matrix_profile_cfg *profile = &display_profiles[i];

        if (profile->profile_id >= DISPLAY_PROFILE_COUNT) {
            LOG_ERR("Profile id out of range: %u (max %d)", profile->profile_id,
                    (int)DISPLAY_PROFILE_COUNT - 1);
            return -EINVAL;
        }

        if (seen_profile_ids[profile->profile_id]) {
            LOG_ERR("Duplicate profile id: %u", profile->profile_id);
            return -EINVAL;
        }
        seen_profile_ids[profile->profile_id] = true;

        if (profile->renderer >= LED_MATRIX_RENDERER_COUNT) {
            LOG_ERR("Profile %u has invalid renderer %u", profile->profile_id, profile->renderer);
            return -EINVAL;
        }

#if !HAS_MATRIX_LED_GRID
        if (profile->renderer == LED_MATRIX_RENDERER_KEYPRESS ||
            profile->renderer == LED_MATRIX_RENDERER_RIPPLE ||
            profile->renderer == LED_MATRIX_RENDERER_RAIN) {
            LOG_ERR("Profile %u renderer %s requires matrix-led-grid", profile->profile_id,
                    renderer_name(profile->renderer));
            return -EINVAL;
        }
#endif

        if (profile->keypress_afterglow_ms < 0) {
            LOG_ERR("Profile %u keypress-afterglow-ms must be >= 0, got %d", profile->profile_id,
                    profile->keypress_afterglow_ms);
            return -EINVAL;
        }

        if (profile->keypress_fadeout_ms < 0) {
            LOG_ERR("Profile %u keypress-fadeout-ms must be >= 0, got %d", profile->profile_id,
                    profile->keypress_fadeout_ms);
            return -EINVAL;
        }

        if (profile->keypress_range < 0) {
            LOG_ERR("Profile %u keypress-range must be >= 0, got %d", profile->profile_id,
                    profile->keypress_range);
            return -EINVAL;
        }

        if (profile->brightness > 100U) {
            LOG_ERR("Profile %u brightness out of range: %u", profile->profile_id,
                    profile->brightness);
            return -EINVAL;
        }

        if (profile->color_map_count != 1U && profile->color_map_count != LOGICAL_LED_COUNT) {
            LOG_ERR("Profile %u color-map length must be 1 or %d, got %d", profile->profile_id,
                    LOGICAL_LED_COUNT, (int)profile->color_map_count);
            return -EINVAL;
        }

        for (size_t j = 0; j < profile->color_map_count; j++) {
            if (!led_matrix_is_valid_configured_color(profile->color_map[j])) {
                LOG_ERR("Profile %u uses unsupported color at %d: 0x%08x", profile->profile_id,
                        (int)j, (unsigned int)profile->color_map[j]);
                return -EINVAL;
            }
        }
    }

    for (size_t id = 0; id < DISPLAY_PROFILE_COUNT; id++) {
        if (!seen_profile_ids[id]) {
            LOG_ERR("Missing profile id %d", (int)id);
            return -EINVAL;
        }
    }

    return 0;
}

static void led_matrix_sanitize_runtime_state(void) {
    if (!led_matrix_has_profiles() || find_display_profile(state.selected_profile_id) == NULL) {
        state.selected_profile_id = 0U;
    }

    for (size_t id = 0; id < INDEPENDENT_GROUP_COUNT; id++) {
        const struct led_matrix_independent_group_cfg *group = find_independent_group(id);
        if (group == NULL ||
            find_independent_profile(group, selected_independent_profile_ids[id]) == NULL) {
            selected_independent_profile_ids[id] = 0U;
        }
    }

    state.brightness = MIN(state.brightness, (uint8_t)100);
    state.speed = led_matrix_normalize_speed(state.speed);
    state.manual_hue_index = led_matrix_normalize_hue_index(state.manual_hue_index);
}

static int validate_layer_colors(void) {
    bool seen_layers[ZMK_KEYMAP_LAYERS_LEN];
    memset(seen_layers, 0, sizeof(seen_layers));

    for (size_t i = 0; i < LAYER_COLOR_COUNT; i++) {
        const struct led_matrix_layer_color_cfg *cfg = &layer_colors[i];
        struct led_zone zone = led_matrix_layer_color_zone_for_cfg(cfg);

        if (cfg->layer_id >= ZMK_KEYMAP_LAYERS_LEN) {
            LOG_ERR("Layer color id out of range: %u", cfg->layer_id);
            return -EINVAL;
        }

        if (seen_layers[cfg->layer_id]) {
            LOG_ERR("Duplicate layer color id: %u", cfg->layer_id);
            return -EINVAL;
        }
        seen_layers[cfg->layer_id] = true;

        if (cfg->brightness > 100U) {
            LOG_ERR("Layer color %u brightness out of range: %u", cfg->layer_id, cfg->brightness);
            return -EINVAL;
        }

        if (!cfg->has_explicit_layer_leds && !led_matrix_has_spatial_grid()) {
            LOG_ERR("Layer color %u requires matrix-led-grid or explicit layer-leds",
                    cfg->layer_id);
            return -EINVAL;
        }

        if (cfg->has_explicit_layer_leds) {
            int err = validate_unique_zone_indices("layer-leds", &zone);
            if (err != 0) {
                return err;
            }
        }

        if (zone.count == 0U) {
            LOG_ERR("Layer color %u has no target LEDs", cfg->layer_id);
            return -EINVAL;
        }

        if (cfg->layer_color_count != 1U && cfg->layer_color_count != zone.count) {
            LOG_ERR("Layer color %u layer-colors length must be 1 or %d, got %d", cfg->layer_id,
                    (int)zone.count, (int)cfg->layer_color_count);
            return -EINVAL;
        }

        for (size_t j = 0; j < cfg->layer_color_count; j++) {
            if (!led_matrix_is_valid_configured_color(cfg->layer_colors[j])) {
                LOG_ERR("Layer color %u uses unsupported color at %d: 0x%08x", cfg->layer_id,
                        (int)j, (unsigned int)cfg->layer_colors[j]);
                return -EINVAL;
            }
        }
    }

    return 0;
}

static int validate_renderable_source_presence(void) {
    if (led_matrix_has_profiles() || led_matrix_has_independent_groups() || LAYER_COLOR_COUNT > 0 ||
        led_matrix_has_renderable_status_zones()) {
        return 0;
    }

    LOG_ERR("zmk,led-matrix requires at least one profile, independent child, layer color, or "
            "status LED zone");
    return -EINVAL;
}

#if LED_DRIVER_IS_MATRIX
#if HAS_LED_INDEXES
static int validate_led_indexes_range(const char *name, const uint16_t *indexes, size_t count,
                                      uint16_t max_index) {
    for (size_t i = 0; i < count; i++) {
        if (indexes[i] >= max_index) {
            LOG_ERR("%s entry out of range: %u (max %u)", name, indexes[i], max_index - 1U);
            return -EINVAL;
        }
    }

    return 0;
}
#endif

static int try_configure_rgb_set_color_sink(const struct led_driver_api *api) {
    if (api->get_info == NULL || api->set_color == NULL || LED_INDEXES_MAP_CHANNELS) {
        return -ENOTSUP;
    }

#if HAS_LED_INDEXES
    if (LED_INDEXES_MAP_LEDS) {
        int err = validate_led_indexes_range("led-indexes", led_indexes, ARRAY_SIZE(led_indexes),
                                             OUTPUT_DEVICE_LED_COUNT);
        if (err != 0) {
            return err;
        }
    }
#endif

    if (!HAS_LED_INDEXES && OUTPUT_DEVICE_LED_COUNT != LOGICAL_LED_COUNT) {
        return -ENOTSUP;
    }

    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        uint16_t physical_led = led;
        const struct led_info *info = NULL;
        struct led_matrix_rgb_sink_entry *entry = &matrix_output_sink.rgb_leds[led];
        int err;

#if HAS_LED_INDEXES
        if (LED_INDEXES_MAP_LEDS) {
            physical_led = led_indexes[led];
        }
#endif

        err = led_get_info(output_dev, physical_led, &info);
        if (err != 0) {
            LOG_ERR("Failed getting LED info at %u (%d)", physical_led, err);
            return err;
        }

        if (!map_rgb_color_slots(info, &entry->red_slot, &entry->green_slot, &entry->blue_slot)) {
            LOG_ERR("LED matrix backend \"%s\" LED %u is not RGB or has no usable color-mapping",
                    output_dev->name, physical_led);
            return -EINVAL;
        }

        entry->led = physical_led;
    }

    matrix_output_sink.kind = LED_MATRIX_MATRIX_SINK_RGB_SET_COLOR;
    matrix_output_sink.name = "rgb-set-color";
    matrix_output_sink.mapped_led_count = LOGICAL_LED_COUNT;
    return 0;
}

static int configure_scalar_sink(bool binary_only) {
#if HAS_LED_INDEXES
    int err = validate_led_indexes_range("led-indexes", led_indexes, ARRAY_SIZE(led_indexes),
                                         OUTPUT_DEVICE_LED_COUNT);
    if (err != 0) {
        return err;
    }

    if (LED_INDEXES_MAP_CHANNELS) {
        for (size_t i = 0; i < LOGICAL_CHANNEL_COUNT; i++) {
            matrix_output_sink.scalar_channels[i] = led_indexes[i];
        }

        matrix_output_sink.kind = binary_only ? LED_MATRIX_MATRIX_SINK_BINARY_TRIPLET
                                              : LED_MATRIX_MATRIX_SINK_SCALAR_TRIPLET;
        matrix_output_sink.name = binary_only ? "binary-triplet" : "scalar-triplet";
        matrix_output_sink.mapped_led_count = LOGICAL_CHANNEL_COUNT;
        return 0;
    }

    if (LED_INDEXES_MAP_LEDS) {
        for (size_t i = 0; i < LOGICAL_LED_COUNT; i++) {
            matrix_output_sink.scalar_leds[i] = led_indexes[i];
        }

        matrix_output_sink.kind =
            binary_only ? LED_MATRIX_MATRIX_SINK_BINARY_LED : LED_MATRIX_MATRIX_SINK_SCALAR_LED;
        matrix_output_sink.name = binary_only ? "binary-led" : "scalar-led";
        matrix_output_sink.mapped_led_count = LOGICAL_LED_COUNT;
        return 0;
    }
#endif

    if (OUTPUT_DEVICE_LED_COUNT != LOGICAL_LED_COUNT) {
        LOG_ERR("Non-channel LED matrix backend \"%s\" requires led-indexes when device LED count "
                "is %u",
                output_dev->name, OUTPUT_DEVICE_LED_COUNT);
        return -EINVAL;
    }

    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        matrix_output_sink.scalar_leds[led] = led;
    }

    matrix_output_sink.kind =
        binary_only ? LED_MATRIX_MATRIX_SINK_BINARY_LED : LED_MATRIX_MATRIX_SINK_SCALAR_LED;
    matrix_output_sink.name = binary_only ? "binary-led" : "scalar-led";
    matrix_output_sink.mapped_led_count = LOGICAL_LED_COUNT;
    return 0;
}

static int configure_matrix_output_sink(void) {
    const struct led_driver_api *api = (const struct led_driver_api *)output_dev->api;
    int err;

    memset(&matrix_output_sink, 0, sizeof(matrix_output_sink));

    if (api == NULL) {
        LOG_ERR("LED matrix output device \"%s\" has no LED API", output_dev->name);
        return -ENOTSUP;
    }

    if (api->write_channels != NULL) {
        if (HAS_LED_INDEXES) {
            LOG_ERR("zmk,led-matrix led-indexes is only supported for non-write_channels backends");
            return -EINVAL;
        }

        matrix_output_sink.kind = LED_MATRIX_MATRIX_SINK_CHANNELS;
        matrix_output_sink.name = "channels";
        matrix_output_sink.mapped_led_count = OUTPUT_CHANNEL_COUNT;
        return 0;
    }

    if (HAS_CHANNEL_INDEXES || HAS_OUTPUT_CHANNEL_COUNT_PROP) {
        LOG_ERR(
            "zmk,led-matrix channel-indexes/output-channel-count require a write_channels backend");
        return -EINVAL;
    }

    if (OUTPUT_DEVICE_LED_COUNT == 0U) {
        LOG_ERR("LED matrix backend \"%s\" needs child LEDs or write_channels support",
                output_dev->name);
        return -ENOTSUP;
    }

    err = try_configure_rgb_set_color_sink(api);
    if (err == 0) {
        return 0;
    }
    if (err != -ENOTSUP) {
        return err;
    }

    if (api->set_brightness != NULL) {
        return configure_scalar_sink(false);
    }

    if (api->on != NULL && api->off != NULL) {
        return configure_scalar_sink(true);
    }

    LOG_ERR(
        "LED matrix backend \"%s\" does not support write_channels/set_color/set_brightness/on-off",
        output_dev->name);
    return -ENOTSUP;
}
#endif

static int validate_dts_layout(void) {
#if DT_NODE_HAS_PROP(MATRIX_NODE, independent_leds)
    LOG_ERR(
        "Top-level independent-leds is deprecated; define independent child containers instead");
    return -EINVAL;
#endif

    int err = validate_status_zone_configs();
    if (err != 0) {
        return err;
    }

    err = validate_status_color_configs();
    if (err != 0) {
        return err;
    }

#if HAS_CHANNEL_INDEXES
    for (size_t i = 0; i < ARRAY_SIZE(channel_indexes); i++) {
        if (channel_indexes[i] >= OUTPUT_CHANNEL_COUNT) {
            LOG_ERR("channel-indexes entry out of range: %u (max %u)", channel_indexes[i],
                    OUTPUT_CHANNEL_COUNT - 1U);
            return -EINVAL;
        }
    }
#endif

    err = validate_spatial_layout();
    if (err != 0) {
        return err;
    }

    err = validate_independent_groups();
    if (err != 0) {
        return err;
    }

    err = validate_profiles();
    if (err != 0) {
        return err;
    }

    err = validate_layer_colors();
    if (err != 0) {
        return err;
    }

    return validate_renderable_source_presence();
}

#include "led_matrix/render.c"

#include "led_matrix/api.c"
