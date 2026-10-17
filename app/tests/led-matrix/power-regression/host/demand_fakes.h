/* SPDX-License-Identifier: MIT */
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dt-bindings/zmk/led_matrix_colors.h>
#include "src/led_matrix/internal.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BIT(n) (1U << (n))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ARG_UNUSED(a) ((void)(a))
#define LOGICAL_LED_COUNT 4
#define ZMK_KEYMAP_LEN 4
#define INDEPENDENT_GROUP_COUNT 1
#define LED_MATRIX_LOCAL_ROWS 2
#define LED_MATRIX_LOCAL_COLS 2
#define LED_DRIVER_IS_MATRIX 1
#define LED_DRIVER_IS_STRIP 0
#define LM_NOLED UINT16_MAX
#define SPECTRUM_HUE_MAX 360U
#define RIPPLE_TAIL_CELLS 2U
#define RAIN_TAIL_CELLS 2U
#define RAIN_TAIL_NEAR_INTENSITY 44U
#define RAIN_TAIL_FAR_INTENSITY 20U
#define RAIN_MIN_SPEED_PCT 80U
#define RAIN_MAX_SPEED_PCT 140U
#define STARLIGHT_SPARKLE_PERIOD_MULTIPLIER 1U
#define STARLIGHT_SPARKLE_WINDOW_DIVISOR 8U
#define STARLIGHT_SPARKLE_MAX_PCT 70U
#define STARLIGHT_METEOR_PERIOD_MULTIPLIER 1U
#define STARLIGHT_METEOR_MIN_PERIOD_MS 250U
#define STARLIGHT_METEOR_ACTIVE_MODULUS 4U
#define STARLIGHT_METEOR_TAIL_CELLS 4U
#define STARLIGHT_METEOR_TAIL_NEAR_INTENSITY 80U
#define STARLIGHT_METEOR_TAIL_MID_INTENSITY 50U
#define STARLIGHT_METEOR_TAIL_FAR_INTENSITY 25U
#define STARLIGHT_METEOR_TAIL_END_INTENSITY 10U
enum {
    LED_MATRIX_RENDERER_SOLID,
    LED_MATRIX_RENDERER_BREATHE,
    LED_MATRIX_RENDERER_SPECTRUM,
    LED_MATRIX_RENDERER_SWIRL,
    LED_MATRIX_RENDERER_KEYPRESS,
    LED_MATRIX_RENDERER_RIPPLE,
    LED_MATRIX_RENDERER_RAIN,
    LED_MATRIX_RENDERER_STARLIGHT
};
enum led_matrix_status_source {
    LED_MATRIX_STATUS_BATTERY = BIT(0),
    LED_MATRIX_STATUS_USB = BIT(1),
    LED_MATRIX_STATUS_BLE = BIT(2),
    LED_MATRIX_STATUS_HID = BIT(3)
};
enum zmk_usb_conn_state { ZMK_USB_CONN_NONE, ZMK_USB_CONN_POWERED, ZMK_USB_CONN_HID };
enum {
    LED_MATRIX_MATRIX_SINK_NONE,
    LED_MATRIX_MATRIX_SINK_CHANNELS,
    LED_MATRIX_MATRIX_SINK_RGB_SET_COLOR,
    LED_MATRIX_MATRIX_SINK_SCALAR_LED,
    LED_MATRIX_MATRIX_SINK_SCALAR_TRIPLET,
    LED_MATRIX_MATRIX_SINK_BINARY_LED,
    LED_MATRIX_MATRIX_SINK_BINARY_TRIPLET
};
struct led_rgb8 {
    uint8_t r, g, b;
};
struct led_hsb {
    uint16_t h;
    uint8_t s, b;
};
struct led_zone {
    const uint16_t *leds;
    size_t count;
};
struct led_matrix_color_list_cfg {
    const uint32_t *colors;
    size_t count;
    uint32_t default_color;
};
struct led_matrix_profile_cfg {
    uint8_t renderer;
    int32_t keypress_afterglow_ms, keypress_fadeout_ms, keypress_range;
    uint16_t period_ms, brightness;
    const uint32_t *color_map;
    size_t color_map_count;
};
struct led_matrix_independent_profile_cfg {
    uint8_t renderer;
    uint16_t period_ms, brightness;
    const uint32_t *color_map;
    size_t color_map_count;
};
struct led_matrix_independent_group_cfg {
    const uint16_t *leds;
    size_t led_count;
};
struct led_matrix_layer_color_cfg {
    uint16_t brightness;
    const uint32_t *layer_colors;
    size_t layer_color_count;
    const uint16_t *layer_leds;
    size_t layer_led_count;
};
struct led_matrix_snapshot {
    uint8_t battery_soc, ble_profile, hid;
    enum zmk_usb_conn_state usb_state;
    bool ble_connected, ble_open;
};
struct led_matrix_hid_indicator_descriptor {
    const struct led_zone *zone;
    const struct led_matrix_color_list_cfg *color_on, *color_off;
    uint8_t bit;
};
struct led_matrix_status_source_descriptor {
    enum led_matrix_status_source source;
    void (*render_overlay)(uint32_t now_ms);
};
struct led_matrix_rain_column_state {
    bool active;
    uint8_t streak_cells, speed_pct;
    uint32_t age_x100;
};
struct led_matrix_rain_state {
    bool initialized;
    uint32_t step_ms, last_step;
    struct led_matrix_rain_column_state columns[LED_MATRIX_LOCAL_COLS];
};
static struct {
    bool on;
    uint8_t brightness;
} state;
static bool force_show_active, sleeping, idle_suspended, profile_present, independent_present;
static bool ble_pairing_transient_active;
static uint8_t source_mask;
static struct led_matrix_snapshot snapshot;
static struct led_rgb8 logical_leds[LOGICAL_LED_COUNT];
static bool logical_leds_dynamic[LOGICAL_LED_COUNT], pressed_positions[ZMK_KEYMAP_LEN];
static uint32_t keypress_release_fade_start_ms[ZMK_KEYMAP_LEN];
static uint32_t keypress_release_deadline_ms[ZMK_KEYMAP_LEN];
static struct {
    bool active;
    uint16_t origin_cell;
    uint32_t start_time_ms;
} ripple_state;
static struct led_matrix_rain_state rain_state;
static uint16_t position_to_led[] = {0, 1, 2, 3}, position_to_cell[] = {0, 1, 2, 3};
static uint16_t cell_to_led[] = {0, 1, 2, 3}, cell_row_idx[] = {0, 0, 1, 1};
static uint16_t cell_col_idx[] = {0, 1, 0, 1};
static uint16_t grid_cell_count = 4, grid_row_count = 2, grid_col_count = 2;
static uint16_t all_leds[] = {0, 1, 2, 3};
static struct {
    int kind;
} matrix_output_sink;
static uint32_t base_colors[LOGICAL_LED_COUNT], independent_colors[LOGICAL_LED_COUNT];
static uint32_t layer_colors[LOGICAL_LED_COUNT], on_colors[5], off_colors[5];
static uint32_t usb_colors[3], ble_colors[3], battery_colors[3];
static struct led_matrix_profile_cfg profile;
static struct led_matrix_independent_profile_cfg independent_profile;
static struct led_matrix_independent_group_cfg independent_groups[INDEPENDENT_GROUP_COUNT];
static struct led_matrix_layer_color_cfg layer_cfg;
static struct {
    bool active;
    const struct led_matrix_layer_color_cfg *cfg;
} layer_color;
static struct led_zone battery_zone, usb_zone, ble_zone, hid_zones[5];
static struct led_matrix_color_list_cfg battery_color_on_cfg, battery_color_off_cfg;
static struct led_matrix_color_list_cfg battery_level_color_cfg;
static struct led_matrix_color_list_cfg usb_color_hid_cfg, usb_color_powered_cfg,
    usb_color_none_cfg;
static struct led_matrix_color_list_cfg ble_color_open_cfg, ble_color_connected_cfg,
    ble_color_disconnected_cfg;
static struct led_matrix_color_list_cfg hid_on_cfg[5], hid_off_cfg[5];
static struct led_matrix_hid_indicator_descriptor hid_indicator_descriptors[5];
static void render_battery_overlay(uint32_t now_ms);
static void render_usb_overlay(uint32_t now_ms);
static void render_ble_overlay(uint32_t now_ms);
static void render_hid_overlay(uint32_t now_ms);
static const struct led_matrix_status_source_descriptor status_source_descriptors[] = {
    {LED_MATRIX_STATUS_BATTERY, render_battery_overlay},
    {LED_MATRIX_STATUS_USB, render_usb_overlay},
    {LED_MATRIX_STATUS_BLE, render_ble_overlay},
    {LED_MATRIX_STATUS_HID, render_hid_overlay}};

/* Only configuration selection/runtime gates are faked. Color math, renderers,
 * compositing, potential-output flags and final demand are production code. */
static bool led_matrix_base_render_enabled(void) {
    return state.on && !sleeping && !idle_suspended;
}
static bool led_matrix_render_enabled(void) {
    return !sleeping && (led_matrix_base_render_enabled() || force_show_active);
}
static bool led_matrix_has_independent_groups(void) { return independent_present; }
static const struct led_matrix_profile_cfg *selected_profile_cfg(void) {
    return profile_present ? &profile : NULL;
}
static const struct led_matrix_independent_profile_cfg *
selected_independent_profile_cfg(const struct led_matrix_independent_group_cfg *g) {
    (void)g;
    return independent_present ? &independent_profile : NULL;
}
static uint8_t current_status_sources(void) {
    return led_matrix_render_enabled() ? source_mask : 0;
}
static struct led_zone
led_matrix_layer_color_zone_for_cfg(const struct led_matrix_layer_color_cfg *c) {
    return (struct led_zone){c->layer_leds, c->layer_led_count};
}
static uint32_t led_matrix_effective_profile_period_ms(const struct led_matrix_profile_cfg *p) {
    return MAX(p->period_ms, 1U);
}
static uint32_t led_matrix_effective_independent_profile_period_ms(
    const struct led_matrix_independent_profile_cfg *p) {
    return MAX(p->period_ms, 1U);
}
static uint8_t led_matrix_dynamic_hue_index(void) { return 0; }
static uint32_t led_matrix_dynamic_period_ms(void) { return 2400U; }
static uint32_t test_now_ms;
static unsigned scheduled_renders;
static uint32_t k_uptime_get_32(void) { return test_now_ms; }
static void led_matrix_schedule_render(void) { scheduled_renders++; }
static bool led_matrix_auto_resume_if_needed(void) {
    if (state.on && idle_suspended && !sleeping) {
        idle_suspended = false;
        return true;
    }
    return false;
}
