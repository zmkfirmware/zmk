/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "internal.h"

#define STATUS_TIMEOUT_MS DT_PROP_OR(MATRIX_NODE, status_timeout_ms, 3000)
#define USB_TIMEOUT_MS DT_PROP_OR(MATRIX_NODE, usb_timeout_ms, STATUS_TIMEOUT_MS)
#define BLE_TIMEOUT_MS DT_PROP_OR(MATRIX_NODE, ble_timeout_ms, STATUS_TIMEOUT_MS)
#define HID_TIMEOUT_MS DT_PROP_OR(MATRIX_NODE, hid_timeout_ms, STATUS_TIMEOUT_MS)
#define BATTERY_TIMEOUT_MS DT_PROP_OR(MATRIX_NODE, battery_timeout_ms, STATUS_TIMEOUT_MS)
#define BLE_PAIRING_HOLD DT_PROP_OR(MATRIX_NODE, ble_pairing_hold, 1)
#define BLE_PAIRING_TIMEOUT_MULTIPLIER 5U

#define LED_MATRIX_TICK_MS 50
#define LED_MATRIX_SPEED_MIN 1
#define LED_MATRIX_SPEED_MAX CONFIG_ZMK_LED_MATRIX_SPD_LEVELS
#define LED_MATRIX_HUE_INDEX_MAX 11
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

#define LED_MATRIX_STATUS_ONLY_PERSISTENT_TIMEOUT_MS                                               \
    ((STATUS_TIMEOUT_MS > 0U) ? STATUS_TIMEOUT_MS : 3000U)

#define LED_MATRIX_SERVICE_SYNC_OUTPUT BIT(0)
#define LED_MATRIX_SERVICE_RENDER BIT(1)

#define DEFAULT_USB_COLOR_HID LM_USB_HID
#define DEFAULT_USB_COLOR_POWERED LM_USB_POWERED
#define DEFAULT_USB_COLOR_NONE LM_USB_NONE

#define DEFAULT_BLE_COLOR_OPEN LM_BLE_OPEN
#define DEFAULT_BLE_COLOR_CONNECTED LM_BLE_CONNECTED
#define DEFAULT_BLE_COLOR_DISCONNECTED LM_BLE_DISCONNECTED

#define DEFAULT_BATTERY_COLOR_ON LM_BATTERY_ON
#define DEFAULT_BATTERY_COLOR_OFF LM_BATTERY_OFF

#define DEFAULT_HID_COLOR_ON LM_HID_ON
#define DEFAULT_HID_COLOR_OFF LM_HID_OFF

#define HID_LED_NUMLOCK_BIT BIT(0)
#define HID_LED_CAPSLOCK_BIT BIT(1)
#define HID_LED_SCROLLLOCK_BIT BIT(2)
#define HID_LED_COMPOSE_BIT BIT(3)
#define HID_LED_KANA_BIT BIT(4)

#define HAS_OUTPUT_CHANNEL_COUNT_PROP DT_NODE_HAS_PROP(MATRIX_NODE, output_channel_count)
#define OUTPUT_PIXEL_COUNT DIV_ROUND_UP(OUTPUT_CHANNEL_COUNT, RGB_CHANNEL_COUNT_PER_LED)

#if DT_NODE_HAS_PROP(MATRIX_NODE, channel_indexes)
static const uint16_t channel_indexes[] = DT_PROP(MATRIX_NODE, channel_indexes);
#define HAS_CHANNEL_INDEXES 1
BUILD_ASSERT(ARRAY_SIZE(channel_indexes) == LOGICAL_CHANNEL_COUNT,
             "zmk,led-matrix channel-indexes length must equal led-count*3");
#else
#define HAS_CHANNEL_INDEXES 0
#endif

#if DT_NODE_HAS_PROP(MATRIX_NODE, led_indexes)
static const uint16_t led_indexes[] = DT_PROP(MATRIX_NODE, led_indexes);
#define HAS_LED_INDEXES 1
BUILD_ASSERT(ARRAY_SIZE(led_indexes) == LOGICAL_LED_COUNT ||
                 ARRAY_SIZE(led_indexes) == LOGICAL_CHANNEL_COUNT,
             "zmk,led-matrix led-indexes length must equal led-count or led-count*3");
#define LED_INDEXES_MAP_LEDS (ARRAY_SIZE(led_indexes) == LOGICAL_LED_COUNT)
#define LED_INDEXES_MAP_CHANNELS (ARRAY_SIZE(led_indexes) == LOGICAL_CHANNEL_COUNT)
#else
#define HAS_LED_INDEXES 0
#define LED_INDEXES_MAP_LEDS 0
#define LED_INDEXES_MAP_CHANNELS 0
#endif

#if DT_NODE_HAS_PROP(MATRIX_NODE, matrix_led_grid)
static const uint16_t matrix_led_grid[] = DT_PROP(MATRIX_NODE, matrix_led_grid);
#define HAS_MATRIX_LED_GRID 1
#define MATRIX_GRID_TOKEN_COUNT ARRAY_SIZE(matrix_led_grid)
#define GRID_CELL_STORAGE_COUNT MAX(1, MATRIX_GRID_TOKEN_COUNT)
BUILD_ASSERT(MATRIX_GRID_TOKEN_COUNT > 0, "zmk,led-matrix matrix-led-grid must not be empty");
#else
#define HAS_MATRIX_LED_GRID 0
#define MATRIX_GRID_TOKEN_COUNT 0
#define GRID_CELL_STORAGE_COUNT 1
#endif

#if DT_NODE_HAS_PROP(ZMK_MATRIX_NODE_ID, row_gpios)
#define LED_MATRIX_LOCAL_ROWS DT_PROP_LEN(ZMK_MATRIX_NODE_ID, row_gpios)
#define LED_MATRIX_LOCAL_COLS DT_PROP_LEN(ZMK_MATRIX_NODE_ID, col_gpios)
#elif DT_NODE_HAS_PROP(ZMK_MATRIX_NODE_ID, input_gpios)
#define LED_MATRIX_LOCAL_ROWS 1
#define LED_MATRIX_LOCAL_COLS DT_PROP_LEN(ZMK_MATRIX_NODE_ID, input_gpios)
#else
#define LED_MATRIX_LOCAL_ROWS DT_PROP(ZMK_MATRIX_NODE_ID, rows)
#define LED_MATRIX_LOCAL_COLS DT_PROP(ZMK_MATRIX_NODE_ID, columns)
#endif

#if DT_HAS_CHOSEN(zmk_matrix_transform)
ZMK_MATRIX_TRANSFORM_EXTERN(DT_CHOSEN(zmk_matrix_transform));
#define LED_MATRIX_TRANSFORM ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_CHOSEN(zmk_matrix_transform))
#elif DT_HAS_CHOSEN(zmk_physical_layout)
ZMK_MATRIX_TRANSFORM_EXTERN(DT_PHANDLE(DT_CHOSEN(zmk_physical_layout), transform));
#define LED_MATRIX_TRANSFORM                                                                       \
    ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_PHANDLE(DT_CHOSEN(zmk_physical_layout), transform))
#else
ZMK_MATRIX_TRANSFORM_DEFAULT_EXTERN();
#define LED_MATRIX_TRANSFORM (&zmk_matrix_transform_default)
#endif

#define DECLARE_U16_ARRAY_OR(node_id, prop, name, fallback)                                        \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop),                                                   \
                (static const uint16_t name[] = DT_PROP(node_id, prop);),                          \
                (static const uint16_t name[] = {fallback};))

#define DECLARE_U32_ARRAY_OR(node_id, prop, name, fallback)                                        \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop),                                                   \
                (static const uint32_t name[] = DT_PROP(node_id, prop);),                          \
                (static const uint32_t name[] = {fallback};))

DECLARE_U16_ARRAY_OR(MATRIX_NODE, battery_leds, battery_leds_dt, LM_NOLED);
DECLARE_U16_ARRAY_OR(MATRIX_NODE, ble_profile_leds, ble_profile_leds_dt, LM_NOLED);
DECLARE_U16_ARRAY_OR(MATRIX_NODE, usb_leds, usb_leds_dt, LM_NOLED);
DECLARE_U16_ARRAY_OR(MATRIX_NODE, hid_caps_leds, hid_caps_leds_dt, LM_NOLED);
DECLARE_U16_ARRAY_OR(MATRIX_NODE, hid_num_leds, hid_num_leds_dt, LM_NOLED);
DECLARE_U16_ARRAY_OR(MATRIX_NODE, hid_scroll_leds, hid_scroll_leds_dt, LM_NOLED);
DECLARE_U16_ARRAY_OR(MATRIX_NODE, hid_compose_leds, hid_compose_leds_dt, LM_NOLED);
DECLARE_U16_ARRAY_OR(MATRIX_NODE, hid_kana_leds, hid_kana_leds_dt, LM_NOLED);

DECLARE_U32_ARRAY_OR(MATRIX_NODE, usb_color_hid, usb_color_hid_dt, DEFAULT_USB_COLOR_HID);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, usb_color_powered, usb_color_powered_dt,
                     DEFAULT_USB_COLOR_POWERED);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, usb_color_none, usb_color_none_dt, DEFAULT_USB_COLOR_NONE);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, ble_color_open, ble_color_open_dt, DEFAULT_BLE_COLOR_OPEN);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, ble_color_connected, ble_color_connected_dt,
                     DEFAULT_BLE_COLOR_CONNECTED);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, ble_color_disconnected, ble_color_disconnected_dt,
                     DEFAULT_BLE_COLOR_DISCONNECTED);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, battery_color_on, battery_color_on_dt, DEFAULT_BATTERY_COLOR_ON);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, battery_color_off, battery_color_off_dt,
                     DEFAULT_BATTERY_COLOR_OFF);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, battery_level_colors, battery_level_colors_dt,
                     DEFAULT_BATTERY_COLOR_ON);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_caps_color_on, hid_caps_color_on_dt, DEFAULT_HID_COLOR_ON);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_caps_color_off, hid_caps_color_off_dt, DEFAULT_HID_COLOR_OFF);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_num_color_on, hid_num_color_on_dt, DEFAULT_HID_COLOR_ON);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_num_color_off, hid_num_color_off_dt, DEFAULT_HID_COLOR_OFF);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_scroll_color_on, hid_scroll_color_on_dt,
                     DEFAULT_HID_COLOR_ON);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_scroll_color_off, hid_scroll_color_off_dt,
                     DEFAULT_HID_COLOR_OFF);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_compose_color_on, hid_compose_color_on_dt,
                     DEFAULT_HID_COLOR_ON);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_compose_color_off, hid_compose_color_off_dt,
                     DEFAULT_HID_COLOR_OFF);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_kana_color_on, hid_kana_color_on_dt, DEFAULT_HID_COLOR_ON);
DECLARE_U32_ARRAY_OR(MATRIX_NODE, hid_kana_color_off, hid_kana_color_off_dt, DEFAULT_HID_COLOR_OFF);

#define ARRAY_COUNT_IF_PROP(node_id, prop) DT_PROP_LEN_OR(node_id, prop, 0)

enum led_matrix_renderer {
    LED_MATRIX_RENDERER_SOLID = 0,
    LED_MATRIX_RENDERER_BREATHE,
    LED_MATRIX_RENDERER_SPECTRUM,
    LED_MATRIX_RENDERER_SWIRL,
    LED_MATRIX_RENDERER_KEYPRESS,
    LED_MATRIX_RENDERER_RIPPLE,
    LED_MATRIX_RENDERER_RAIN,
    LED_MATRIX_RENDERER_STARLIGHT,
    LED_MATRIX_RENDERER_COUNT,
};

enum led_matrix_status_mode {
    STATUS_MODE_NONE = 0,
    STATUS_MODE_TRANSIENT,
};

enum led_matrix_status_wake_context {
    LED_MATRIX_WAKE_CONTEXT_NONE = 0,
    LED_MATRIX_WAKE_CONTEXT_MANUAL_OFF_TRANSIENT,
    LED_MATRIX_WAKE_CONTEXT_IDLE_AUTO_OFF_STATUS_ONLY,
};

enum led_matrix_status_source {
    LED_MATRIX_STATUS_BATTERY = BIT(0),
    LED_MATRIX_STATUS_USB = BIT(1),
    LED_MATRIX_STATUS_BLE = BIT(2),
    LED_MATRIX_STATUS_HID = BIT(3),
};

#define LED_MATRIX_STATUS_ALL                                                                      \
    (LED_MATRIX_STATUS_BATTERY | LED_MATRIX_STATUS_USB | LED_MATRIX_STATUS_BLE |                   \
     LED_MATRIX_STATUS_HID)

enum led_matrix_matrix_sink_kind {
    LED_MATRIX_MATRIX_SINK_NONE = 0,
    LED_MATRIX_MATRIX_SINK_CHANNELS,
    LED_MATRIX_MATRIX_SINK_RGB_SET_COLOR,
    LED_MATRIX_MATRIX_SINK_SCALAR_LED,
    LED_MATRIX_MATRIX_SINK_SCALAR_TRIPLET,
    LED_MATRIX_MATRIX_SINK_BINARY_LED,
    LED_MATRIX_MATRIX_SINK_BINARY_TRIPLET,
};

struct led_rgb8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct led_hsb {
    uint16_t h;
    uint8_t s;
    uint8_t b;
};

struct led_zone {
    const uint16_t *leds;
    size_t count;
};

struct led_matrix_snapshot;

struct led_matrix_color_list_cfg {
    const uint32_t *colors;
    size_t count;
    uint32_t default_color;
};

struct led_matrix_hid_indicator_descriptor {
    const char *zone_name;
    const char *color_on_name;
    const char *color_off_name;
    const struct led_zone *zone;
    const struct led_matrix_color_list_cfg *color_on;
    const struct led_matrix_color_list_cfg *color_off;
    zmk_hid_indicators_t bit;
};

struct led_matrix_status_source_descriptor {
    enum led_matrix_status_source source;
    const char *name;
    uint16_t timeout_ms;
    bool (*has_zone)(void);
    bool (*snapshot_valid)(const struct led_matrix_snapshot *snapshot);
    bool (*needs_periodic_refresh)(uint32_t now_ms);
    void (*render_overlay)(uint32_t now_ms);
};

struct led_matrix_zone_validation_descriptor {
    const char *name;
    const struct led_zone *zone;
};

struct led_matrix_color_validation_descriptor {
    const char *name;
    const struct led_matrix_color_list_cfg *colors;
    const struct led_zone *expected_zone;
    bool bucket_colors;
};

struct led_matrix_profile_cfg {
    uint8_t profile_id;
    uint8_t renderer;
    int32_t keypress_afterglow_ms;
    int32_t keypress_fadeout_ms;
    int32_t keypress_range;
    uint16_t period_ms;
    uint16_t brightness;
    const uint32_t *color_map;
    size_t color_map_count;
    const char *display_name;
};

struct led_matrix_independent_profile_cfg {
    uint8_t profile_id;
    uint8_t renderer;
    uint16_t period_ms;
    uint16_t brightness;
    const uint32_t *color_map;
    size_t color_map_count;
};

struct led_matrix_independent_group_cfg {
    uint8_t independent_id;
    const uint16_t *leds;
    size_t led_count;
    const struct led_matrix_independent_profile_cfg *profiles;
    size_t profile_count;
};

struct led_matrix_layer_color_cfg {
    zmk_keymap_layer_id_t layer_id;
    uint16_t brightness;
    uint32_t display_time_ms;
    const uint32_t *layer_colors;
    size_t layer_color_count;
    const uint16_t *layer_leds;
    size_t layer_led_count;
    bool has_explicit_layer_leds;
};

struct led_matrix_state {
    bool on;
    uint8_t brightness;
    uint8_t selected_profile_id;
    uint8_t speed;
    uint8_t manual_hue_index;
};

struct led_matrix_settings_v1 {
    uint8_t on;
    uint8_t brightness;
    uint8_t selected_profile_id;
};

struct led_matrix_settings_v2 {
    uint8_t on;
    uint8_t brightness;
    uint8_t selected_profile_id;
    uint8_t speed;
};

struct led_matrix_settings_v3 {
    uint8_t on;
    uint8_t brightness;
    uint8_t selected_profile_id;
    uint8_t speed;
    uint8_t manual_hue_index;
};

struct led_matrix_settings_v4 {
    uint8_t on;
    uint8_t brightness;
    uint8_t selected_profile_id;
    uint8_t speed;
    uint8_t manual_hue_index;
    uint8_t selected_independent_profile_ids[LOGICAL_LED_COUNT];
};

struct led_matrix_snapshot {
    uint8_t battery_soc;
    enum zmk_usb_conn_state usb_state;
    uint8_t ble_profile;
    bool ble_connected;
    bool ble_open;
    zmk_hid_indicators_t hid;
    enum zmk_transport output_transport;
    zmk_keymap_layer_id_t active_layer;
};

struct led_matrix_layer_color_state {
    bool active;
    bool persistent;
    zmk_keymap_layer_id_t layer_id;
    uint32_t deadline_ms;
    const struct led_matrix_layer_color_cfg *cfg;
};

struct led_matrix_ripple_state {
    bool active;
    uint16_t origin_cell;
    uint32_t start_time_ms;
};

struct led_matrix_rain_column_state {
    bool active;
    uint8_t streak_cells;
    uint8_t speed_pct;
    uint32_t age_x100;
};

struct led_matrix_rain_state {
    bool initialized;
    uint32_t step_ms;
    uint32_t last_step;
    struct led_matrix_rain_column_state columns[LED_MATRIX_LOCAL_COLS];
};

#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
struct led_matrix_idle_auto_off_state {
    bool suspended;
    bool restore_on_wake;
};
#endif

struct led_matrix_idle_status_wake_state {
    bool idle_started_at_valid;
    uint32_t idle_started_at_ms;
};

struct led_matrix_rgb_sink_entry {
    uint16_t led;
    uint8_t red_slot;
    uint8_t green_slot;
    uint8_t blue_slot;
};

struct led_matrix_output_sink {
    enum led_matrix_matrix_sink_kind kind;
    const char *name;
    uint16_t mapped_led_count;
    struct led_matrix_rgb_sink_entry rgb_leds[LOGICAL_LED_COUNT];
    uint16_t scalar_leds[LOGICAL_LED_COUNT];
    uint16_t scalar_channels[LOGICAL_CHANNEL_COUNT];
};

#define DISPLAY_PROFILE_CHILD_COUNT(node_id)                                                       \
    +COND_CODE_1(DT_NODE_HAS_PROP(node_id, profile_id), (1), (0))
#define INDEPENDENT_GROUP_CHILD_COUNT(node_id)                                                     \
    +COND_CODE_1(DT_NODE_HAS_PROP(node_id, independent_id), (1), (0))
#define LAYER_COLOR_CHILD_COUNT(node_id) +COND_CODE_1(DT_NODE_HAS_PROP(node_id, layer_id), (1), (0))

#define DISPLAY_PROFILE_COUNT                                                                      \
    (0 DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, DISPLAY_PROFILE_CHILD_COUNT))
#define INDEPENDENT_GROUP_COUNT                                                                    \
    (0 DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, INDEPENDENT_GROUP_CHILD_COUNT))
#define LAYER_COLOR_COUNT (0 DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, LAYER_COLOR_CHILD_COUNT))
#define LED_MATRIX_INDEPENDENT_PROFILE_SLOT_COUNT MAX(1, INDEPENDENT_GROUP_COUNT)

struct led_matrix_settings_v5 {
    uint8_t on;
    uint8_t brightness;
    uint8_t selected_profile_id;
    uint8_t speed;
    uint8_t manual_hue_index;
    uint8_t selected_independent_profile_ids[LED_MATRIX_INDEPENDENT_PROFILE_SLOT_COUNT];
};

#define LED_MATRIX_PROFILE_COLOR_MAP_NAME(node_id)                                                 \
    UTIL_CAT(led_matrix_profile_color_map_, DT_NODE_CHILD_IDX(node_id))
#define LED_MATRIX_PROFILE_DISPLAY_NAME_NAME(node_id)                                              \
    UTIL_CAT(led_matrix_profile_display_name_, DT_NODE_CHILD_IDX(node_id))
#define LED_MATRIX_LAYER_COLOR_MAP_NAME(node_id)                                                   \
    UTIL_CAT(led_matrix_layer_color_map_, DT_NODE_CHILD_IDX(node_id))
#define LED_MATRIX_LAYER_LEDS_NAME(node_id)                                                        \
    UTIL_CAT(led_matrix_layer_leds_, DT_NODE_CHILD_IDX(node_id))
#define LED_MATRIX_INDEPENDENT_LEDS_NAME(node_id)                                                  \
    UTIL_CAT(led_matrix_independent_leds_, DT_NODE_CHILD_IDX(node_id))
#define LED_MATRIX_INDEPENDENT_PROFILES_NAME(node_id)                                              \
    UTIL_CAT(led_matrix_independent_profiles_, DT_NODE_CHILD_IDX(node_id))
#define LED_MATRIX_INDEPENDENT_PROFILE_COLOR_MAP_NAME(group_node, node_id)                         \
    UTIL_CAT(UTIL_CAT(UTIL_CAT(led_matrix_independent_profile_color_map_,                          \
                               DT_NODE_CHILD_IDX(group_node)),                                     \
                      _),                                                                          \
             DT_NODE_CHILD_IDX(node_id))

#define DECLARE_DISPLAY_PROFILE_COLOR_MAP(node_id)                                                 \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, profile_id),                                             \
                (DECLARE_U32_ARRAY_OR(node_id, colors, LED_MATRIX_PROFILE_COLOR_MAP_NAME(node_id), \
                                      LM_BLACK)),                                                  \
                ())

#define DECLARE_DISPLAY_PROFILE_NAME(node_id)                                                      \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, profile_id),                                             \
                (static const char LED_MATRIX_PROFILE_DISPLAY_NAME_NAME(node_id)[] =               \
                     COND_CODE_1(DT_NODE_HAS_PROP(node_id, display_name),                          \
                                 (DT_PROP(node_id, display_name)), (""));),                        \
                ())

DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, DECLARE_DISPLAY_PROFILE_COLOR_MAP)
DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, DECLARE_DISPLAY_PROFILE_NAME)

#define DISPLAY_PROFILE_ENTRY(node_id)                                                             \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, profile_id),                                             \
                (                                                                                  \
                    {                                                                              \
                        .profile_id = DT_PROP(node_id, profile_id),                                \
                        .renderer = DT_ENUM_IDX_OR(node_id, renderer, LED_MATRIX_RENDERER_SOLID),  \
                        .keypress_afterglow_ms = DT_PROP_OR(node_id, keypress_afterglow_ms, 0),    \
                        .keypress_fadeout_ms = DT_PROP_OR(node_id, keypress_fadeout_ms, 0),        \
                        .keypress_range = DT_PROP_OR(node_id, keypress_range, 0),                  \
                        .period_ms = DT_PROP_OR(node_id, period_ms, 1000),                         \
                        .brightness = DT_PROP_OR(node_id, brightness, 100),                        \
                        .color_map = LED_MATRIX_PROFILE_COLOR_MAP_NAME(node_id),                   \
                        .color_map_count = DT_PROP_LEN_OR(node_id, colors, 0),                     \
                        .display_name = LED_MATRIX_PROFILE_DISPLAY_NAME_NAME(node_id),             \
                    }, ),                                                                          \
                ())

#if DISPLAY_PROFILE_COUNT > 0
static const struct led_matrix_profile_cfg display_profiles[] = {
    DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, DISPLAY_PROFILE_ENTRY)};
#else
static const struct led_matrix_profile_cfg display_profiles[1] = {{0}};
#endif

#define DECLARE_INDEPENDENT_LEDS(node_id)                                                          \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, independent_id),                                         \
                (DECLARE_U16_ARRAY_OR(node_id, independent_leds,                                   \
                                      LED_MATRIX_INDEPENDENT_LEDS_NAME(node_id), LM_NOLED)),       \
                ())

#define DECLARE_INDEPENDENT_PROFILE_COLOR_MAP(node_id, group_node)                                 \
    DECLARE_U32_ARRAY_OR(node_id, colors,                                                          \
                         LED_MATRIX_INDEPENDENT_PROFILE_COLOR_MAP_NAME(group_node, node_id),       \
                         LM_BLACK)

#define DECLARE_INDEPENDENT_PROFILE_BLOCK(node_id)                                                 \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, independent_id),                                         \
                (DT_FOREACH_CHILD_STATUS_OKAY_VARGS(                                               \
                     node_id, DECLARE_INDEPENDENT_PROFILE_COLOR_MAP,                               \
                     node_id) static const struct led_matrix_independent_profile_cfg               \
                     LED_MATRIX_INDEPENDENT_PROFILES_NAME(                                         \
                         node_id)[MAX(1, DT_CHILD_NUM_STATUS_OKAY(node_id))] =                     \
                         {COND_CODE_1(UTIL_BOOL(DT_CHILD_NUM_STATUS_OKAY(node_id)),                \
                                      (DT_FOREACH_CHILD_STATUS_OKAY_VARGS(                         \
                                          node_id, INDEPENDENT_PROFILE_ENTRY, node_id)),           \
                                      ({0}))};),                                                   \
                ())

#define INDEPENDENT_PROFILE_ENTRY(node_id, group_node)                                             \
    {                                                                                              \
        .profile_id = DT_PROP_OR(node_id, profile_id, DT_NODE_CHILD_IDX(node_id)),                 \
        .renderer = DT_ENUM_IDX_OR(node_id, renderer, LED_MATRIX_RENDERER_SOLID),                  \
        .period_ms = DT_PROP_OR(node_id, period_ms, 1000),                                         \
        .brightness = DT_PROP_OR(node_id, brightness, 100),                                        \
        .color_map = LED_MATRIX_INDEPENDENT_PROFILE_COLOR_MAP_NAME(group_node, node_id),           \
        .color_map_count = DT_PROP_LEN_OR(node_id, colors, 0),                                     \
    },

DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, DECLARE_INDEPENDENT_LEDS)
DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, DECLARE_INDEPENDENT_PROFILE_BLOCK)

#define INDEPENDENT_GROUP_ENTRY(node_id)                                                           \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, independent_id),                                         \
                (                                                                                  \
                    {                                                                              \
                        .independent_id = DT_PROP(node_id, independent_id),                        \
                        .leds = LED_MATRIX_INDEPENDENT_LEDS_NAME(node_id),                         \
                        .led_count = DT_PROP_LEN_OR(node_id, independent_leds, 0),                 \
                        .profiles = LED_MATRIX_INDEPENDENT_PROFILES_NAME(node_id),                 \
                        .profile_count = DT_CHILD_NUM_STATUS_OKAY(node_id),                        \
                    }, ),                                                                          \
                ())

#if INDEPENDENT_GROUP_COUNT > 0
static const struct led_matrix_independent_group_cfg independent_groups[] = {
    DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, INDEPENDENT_GROUP_ENTRY)};
#else
static const struct led_matrix_independent_group_cfg independent_groups[1] = {{0}};
#endif

#define DECLARE_LAYER_COLOR_MAP(node_id)                                                           \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, layer_id),                                               \
                (DECLARE_U32_ARRAY_OR(node_id, layer_colors,                                       \
                                      LED_MATRIX_LAYER_COLOR_MAP_NAME(node_id), LM_BLACK)          \
                     DECLARE_U16_ARRAY_OR(node_id, layer_leds,                                     \
                                          LED_MATRIX_LAYER_LEDS_NAME(node_id), LM_NOLED)),         \
                ())

DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, DECLARE_LAYER_COLOR_MAP)

#define LAYER_COLOR_ENTRY(node_id)                                                                 \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, layer_id),                                               \
                (                                                                                  \
                    {                                                                              \
                        .layer_id = DT_PROP(node_id, layer_id),                                    \
                        .brightness = DT_PROP_OR(node_id, brightness, 100),                        \
                        .display_time_ms = DT_PROP_OR(node_id, display_time_ms, 1000),             \
                        .layer_colors = LED_MATRIX_LAYER_COLOR_MAP_NAME(node_id),                  \
                        .layer_color_count = DT_PROP_LEN_OR(node_id, layer_colors, 0),             \
                        .layer_leds = LED_MATRIX_LAYER_LEDS_NAME(node_id),                         \
                        .layer_led_count = DT_PROP_LEN_OR(node_id, layer_leds, 0),                 \
                        .has_explicit_layer_leds = DT_NODE_HAS_PROP(node_id, layer_leds),          \
                    }, ),                                                                          \
                ())

#if LAYER_COLOR_COUNT > 0
static const struct led_matrix_layer_color_cfg layer_colors[] = {
    DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, LAYER_COLOR_ENTRY)};
#else
static const struct led_matrix_layer_color_cfg layer_colors[1] = {{0}};
#endif

static struct led_matrix_state state = {
    .on = IS_ENABLED(CONFIG_ZMK_LED_MATRIX_ON_START),
    .brightness = CONFIG_ZMK_LED_MATRIX_BRT_START,
    .selected_profile_id = CONFIG_ZMK_LED_MATRIX_PROFILE_START,
    .speed = CONFIG_ZMK_LED_MATRIX_SPD_START,
    .manual_hue_index = 0,
};

static struct led_matrix_snapshot snapshot;
static struct led_matrix_layer_color_state layer_color;
static enum led_matrix_status_mode status_mode = STATUS_MODE_NONE;
static uint8_t transient_sources;
static bool ble_pairing_transient_active;
static bool ble_pairing_transient_allows_completion_overlay;
static bool force_show_active;
static enum led_matrix_status_wake_context force_show_wake_context;
static bool sleep_suspended;
static uint32_t transient_deadline_ms;
static uint8_t automatic_persistent_status_sources_mask;
static struct led_matrix_ripple_state ripple_state;
static struct led_matrix_rain_state rain_state;
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
static struct led_matrix_idle_auto_off_state idle_auto_off_state;
#endif
static struct led_matrix_idle_status_wake_state idle_status_wake_state;
static struct led_rgb8 logical_leds[LOGICAL_LED_COUNT];
static uint8_t channel_values[OUTPUT_CHANNEL_COUNT];
static bool pressed_positions[ZMK_KEYMAP_LEN];
static uint32_t keypress_release_fade_start_ms[ZMK_KEYMAP_LEN];
static uint32_t keypress_release_deadline_ms[ZMK_KEYMAP_LEN];
static struct led_rgb8 last_flushed_logical_leds[LOGICAL_LED_COUNT];
static bool last_flushed_frame_valid;
static bool led_matrix_tick_running;
static uint8_t last_flushed_brightness;
static uint16_t cell_to_led[GRID_CELL_STORAGE_COUNT];
static uint16_t cell_row_idx[GRID_CELL_STORAGE_COUNT];
static uint16_t cell_col_idx[GRID_CELL_STORAGE_COUNT];
static uint16_t default_layer_color_leds[GRID_CELL_STORAGE_COUNT];
static size_t default_layer_color_led_count;
static uint16_t led_to_cell[LOGICAL_LED_COUNT];
static uint16_t position_to_led[ZMK_KEYMAP_LEN];
static uint16_t position_to_cell[ZMK_KEYMAP_LEN];
static uint8_t selected_independent_profile_ids[LED_MATRIX_INDEPENDENT_PROFILE_SLOT_COUNT];
static size_t grid_cell_count;
static uint16_t grid_row_count;
static uint16_t grid_col_count;
static atomic_t led_matrix_service_requests;

#if LED_DRIVER_IS_MATRIX
static struct led_matrix_output_sink matrix_output_sink;
#endif

#if LED_DRIVER_IS_STRIP
static struct led_rgb strip_pixels[OUTPUT_PIXEL_COUNT];
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EXT_POWER)
static const struct device *const ext_power = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));
#endif
#endif

static void led_matrix_reset_selected_independent_profiles(void) {
    memset(selected_independent_profile_ids, 0, sizeof(selected_independent_profile_ids));
}

static void led_matrix_load_selected_independent_profiles(const uint8_t *saved_ids,
                                                          size_t saved_count) {
    if (saved_ids == NULL || saved_count == 0U) {
        return;
    }

    size_t count = MIN(saved_count, ARRAY_SIZE(selected_independent_profile_ids));
    for (size_t id = 0; id < count; id++) {
        selected_independent_profile_ids[id] = saved_ids[id];
    }
}

static void led_matrix_store_selected_independent_profiles(uint8_t *saved_ids, size_t saved_count) {
    if (saved_ids == NULL || saved_count == 0U) {
        return;
    }

    memset(saved_ids, 0, saved_count * sizeof(saved_ids[0]));

    size_t count = MIN(saved_count, ARRAY_SIZE(selected_independent_profile_ids));
    for (size_t id = 0; id < count; id++) {
        saved_ids[id] = selected_independent_profile_ids[id];
    }
}

static const struct led_zone battery_zone = {
    .leds = battery_leds_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, battery_leds),
};
static const struct led_zone ble_zone = {
    .leds = ble_profile_leds_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, ble_profile_leds),
};
static const struct led_zone usb_zone = {
    .leds = usb_leds_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, usb_leds),
};
static const struct led_zone hid_caps_zone = {
    .leds = hid_caps_leds_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_caps_leds),
};
static const struct led_zone hid_num_zone = {
    .leds = hid_num_leds_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_num_leds),
};
static const struct led_zone hid_scroll_zone = {
    .leds = hid_scroll_leds_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_scroll_leds),
};
static const struct led_zone hid_compose_zone = {
    .leds = hid_compose_leds_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_compose_leds),
};
static const struct led_zone hid_kana_zone = {
    .leds = hid_kana_leds_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_kana_leds),
};

static const struct led_matrix_color_list_cfg usb_color_hid_cfg = {
    .colors = usb_color_hid_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, usb_color_hid),
    .default_color = DEFAULT_USB_COLOR_HID,
};
static const struct led_matrix_color_list_cfg usb_color_powered_cfg = {
    .colors = usb_color_powered_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, usb_color_powered),
    .default_color = DEFAULT_USB_COLOR_POWERED,
};
static const struct led_matrix_color_list_cfg usb_color_none_cfg = {
    .colors = usb_color_none_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, usb_color_none),
    .default_color = DEFAULT_USB_COLOR_NONE,
};
static const struct led_matrix_color_list_cfg ble_color_open_cfg = {
    .colors = ble_color_open_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, ble_color_open),
    .default_color = DEFAULT_BLE_COLOR_OPEN,
};
static const struct led_matrix_color_list_cfg ble_color_connected_cfg = {
    .colors = ble_color_connected_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, ble_color_connected),
    .default_color = DEFAULT_BLE_COLOR_CONNECTED,
};
static const struct led_matrix_color_list_cfg ble_color_disconnected_cfg = {
    .colors = ble_color_disconnected_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, ble_color_disconnected),
    .default_color = DEFAULT_BLE_COLOR_DISCONNECTED,
};
static const struct led_matrix_color_list_cfg battery_color_on_cfg = {
    .colors = battery_color_on_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, battery_color_on),
    .default_color = DEFAULT_BATTERY_COLOR_ON,
};
static const struct led_matrix_color_list_cfg battery_color_off_cfg = {
    .colors = battery_color_off_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, battery_color_off),
    .default_color = DEFAULT_BATTERY_COLOR_OFF,
};
static const struct led_matrix_color_list_cfg battery_level_color_cfg = {
    .colors = battery_level_colors_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, battery_level_colors),
    .default_color = DEFAULT_BATTERY_COLOR_ON,
};
static const struct led_matrix_color_list_cfg hid_caps_color_on_cfg = {
    .colors = hid_caps_color_on_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_caps_color_on),
    .default_color = DEFAULT_HID_COLOR_ON,
};
static const struct led_matrix_color_list_cfg hid_caps_color_off_cfg = {
    .colors = hid_caps_color_off_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_caps_color_off),
    .default_color = DEFAULT_HID_COLOR_OFF,
};
static const struct led_matrix_color_list_cfg hid_num_color_on_cfg = {
    .colors = hid_num_color_on_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_num_color_on),
    .default_color = DEFAULT_HID_COLOR_ON,
};
static const struct led_matrix_color_list_cfg hid_num_color_off_cfg = {
    .colors = hid_num_color_off_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_num_color_off),
    .default_color = DEFAULT_HID_COLOR_OFF,
};
static const struct led_matrix_color_list_cfg hid_scroll_color_on_cfg = {
    .colors = hid_scroll_color_on_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_scroll_color_on),
    .default_color = DEFAULT_HID_COLOR_ON,
};
static const struct led_matrix_color_list_cfg hid_scroll_color_off_cfg = {
    .colors = hid_scroll_color_off_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_scroll_color_off),
    .default_color = DEFAULT_HID_COLOR_OFF,
};
static const struct led_matrix_color_list_cfg hid_compose_color_on_cfg = {
    .colors = hid_compose_color_on_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_compose_color_on),
    .default_color = DEFAULT_HID_COLOR_ON,
};
static const struct led_matrix_color_list_cfg hid_compose_color_off_cfg = {
    .colors = hid_compose_color_off_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_compose_color_off),
    .default_color = DEFAULT_HID_COLOR_OFF,
};
static const struct led_matrix_color_list_cfg hid_kana_color_on_cfg = {
    .colors = hid_kana_color_on_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_kana_color_on),
    .default_color = DEFAULT_HID_COLOR_ON,
};
static const struct led_matrix_color_list_cfg hid_kana_color_off_cfg = {
    .colors = hid_kana_color_off_dt,
    .count = ARRAY_COUNT_IF_PROP(MATRIX_NODE, hid_kana_color_off),
    .default_color = DEFAULT_HID_COLOR_OFF,
};

static bool led_matrix_status_source_has_battery_zone(void) { return battery_zone.count > 0U; }
static bool led_matrix_status_source_has_usb_zone(void) { return usb_zone.count > 0U; }
static bool led_matrix_status_source_has_ble_zone(void) { return ble_zone.count > 0U; }
static bool led_matrix_status_source_has_hid_zone(void) {
    return hid_caps_zone.count > 0U || hid_num_zone.count > 0U || hid_scroll_zone.count > 0U ||
           hid_compose_zone.count > 0U || hid_kana_zone.count > 0U;
}

static bool led_matrix_battery_snapshot_valid(const struct led_matrix_snapshot *snapshot) {
    ARG_UNUSED(snapshot);
    return battery_zone.count > 0U;
}

static bool led_matrix_usb_snapshot_valid(const struct led_matrix_snapshot *snapshot) {
    ARG_UNUSED(snapshot);
    return usb_zone.count > 0U;
}

static bool led_matrix_ble_snapshot_valid(const struct led_matrix_snapshot *snapshot) {
    return ble_zone.count > 0U && snapshot != NULL && snapshot->ble_profile < ble_zone.count;
}

static bool led_matrix_hid_snapshot_valid(const struct led_matrix_snapshot *snapshot) {
    ARG_UNUSED(snapshot);
    return led_matrix_status_source_has_hid_zone();
}

static bool led_matrix_color_is_dynamic(uint32_t color) { return (color & 0xFF000000U) != 0U; }

static bool led_matrix_color_is_time_dynamic(uint32_t color) { return color == LM_RAINBOW; }

static bool led_matrix_color_list_has_time_dynamic_color(const uint32_t *colors, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (led_matrix_color_is_time_dynamic(colors[i])) {
            return true;
        }
    }

    return false;
}

static uint8_t led_matrix_default_speed(void) { return CONFIG_ZMK_LED_MATRIX_SPD_START; }

static uint8_t led_matrix_normalize_speed(uint8_t speed) {
    if (speed == 0U) {
        speed = led_matrix_default_speed();
    }

    return CLAMP(speed, LED_MATRIX_SPEED_MIN, LED_MATRIX_SPEED_MAX);
}

static uint8_t led_matrix_default_hue_index(void) { return 0U; }

static uint8_t led_matrix_normalize_hue_index(uint8_t hue_index) {
    return (uint8_t)(hue_index % (LED_MATRIX_HUE_INDEX_MAX + 1U));
}

static const char *renderer_name(uint8_t renderer) {
    switch (renderer) {
    case LED_MATRIX_RENDERER_SOLID:
        return "solid";
    case LED_MATRIX_RENDERER_BREATHE:
        return "breathe";
    case LED_MATRIX_RENDERER_SPECTRUM:
        return "spectrum";
    case LED_MATRIX_RENDERER_SWIRL:
        return "swirl";
    case LED_MATRIX_RENDERER_KEYPRESS:
        return "keypress";
    case LED_MATRIX_RENDERER_RIPPLE:
        return "ripple";
    case LED_MATRIX_RENDERER_RAIN:
        return "rain";
    case LED_MATRIX_RENDERER_STARLIGHT:
        return "starlight";
    default:
        return "unknown";
    }
}

static bool led_matrix_has_profiles(void) { return DISPLAY_PROFILE_COUNT > 0U; }

static bool led_matrix_has_independent_groups(void) { return INDEPENDENT_GROUP_COUNT > 0U; }

static bool led_matrix_has_spatial_grid(void) { return grid_cell_count > 0U; }

static bool led_matrix_has_renderable_status_zones(void) {
    return battery_zone.count > 0U || usb_zone.count > 0U || ble_zone.count > 0U ||
           led_matrix_status_source_has_hid_zone();
}

static bool led_matrix_is_valid_configured_color(uint32_t color) {
    if ((color & 0xFF000000U) == 0U) {
        return true;
    }

    return color == LM_RAINBOW || color == LM_PALETTE;
}

static uint32_t led_matrix_speed_scaled_period(uint32_t period_ms) {
    uint32_t speed = MAX((uint32_t)LED_MATRIX_SPEED_MIN, (uint32_t)state.speed);
    uint32_t default_speed =
        MAX((uint32_t)LED_MATRIX_SPEED_MIN, (uint32_t)led_matrix_default_speed());
    uint32_t scaled = (period_ms * default_speed) / speed;

    return MAX((uint32_t)1U, scaled);
}

static uint32_t
led_matrix_effective_profile_period_ms(const struct led_matrix_profile_cfg *profile) {
    if (profile == NULL) {
        return 1U;
    }

    return led_matrix_speed_scaled_period(MAX((uint32_t)1U, (uint32_t)profile->period_ms));
}

static uint32_t led_matrix_effective_independent_profile_period_ms(
    const struct led_matrix_independent_profile_cfg *profile) {
    if (profile == NULL) {
        return 1U;
    }

    return led_matrix_speed_scaled_period(MAX((uint32_t)1U, (uint32_t)profile->period_ms));
}

static inline uint16_t led_to_channel(uint16_t led, uint8_t channel) {
#if HAS_CHANNEL_INDEXES
    return channel_indexes[(led * RGB_CHANNEL_COUNT_PER_LED) + channel];
#else
    return (led * RGB_CHANNEL_COUNT_PER_LED) + channel;
#endif
}

static struct led_rgb8 scale_color(struct led_rgb8 color, uint8_t pct) {
    return (struct led_rgb8){
        .r = (uint8_t)(((uint16_t)color.r * pct) / 100U),
        .g = (uint8_t)(((uint16_t)color.g * pct) / 100U),
        .b = (uint8_t)(((uint16_t)color.b * pct) / 100U),
    };
}

static struct led_rgb8 color24_to_rgb8(uint32_t color24) {
    return (struct led_rgb8){
        .r = (uint8_t)((color24 >> 16) & 0xFFU),
        .g = (uint8_t)((color24 >> 8) & 0xFFU),
        .b = (uint8_t)(color24 & 0xFFU),
    };
}

static struct led_hsb rgb8_to_hsb(struct led_rgb8 color) {
    uint8_t max = MAX(color.r, MAX(color.g, color.b));
    uint8_t min = MIN(color.r, MIN(color.g, color.b));
    uint8_t delta = max - min;
    uint16_t hue = 0U;
    uint8_t sat = max == 0U ? 0U : (uint8_t)(((uint16_t)delta * 100U) / max);

    if (delta != 0U) {
        if (max == color.r) {
            hue = (uint16_t)(60 * (((int)color.g - (int)color.b) * 100 / delta)) / 100;
        } else if (max == color.g) {
            hue = (uint16_t)(120 + (60 * (((int)color.b - (int)color.r) * 100 / delta)) / 100);
        } else {
            hue = (uint16_t)(240 + (60 * (((int)color.r - (int)color.g) * 100 / delta)) / 100);
        }

        while ((int16_t)hue < 0) {
            hue = (uint16_t)(hue + 360U);
        }
        hue %= 360U;
    }

    return (struct led_hsb){.h = hue, .s = sat, .b = (uint8_t)(((uint16_t)max * 100U) / 255U)};
}

static struct led_rgb8 hsb_to_rgb8(struct led_hsb color) {
    uint16_t hue = color.h % 360U;
    uint8_t sat = MIN(color.s, (uint8_t)100);
    uint8_t val = MIN(color.b, (uint8_t)100);
    uint16_t c = ((uint16_t)val * sat) / 100U;
    uint16_t h_mod = hue % 120U;
    uint16_t x = (uint16_t)(c * (60U - (uint16_t)abs((int)h_mod - 60)) / 60U);
    uint16_t m = val - c;
    uint16_t r1 = 0U, g1 = 0U, b1 = 0U;

    switch (hue / 60U) {
    case 0:
        r1 = c;
        g1 = x;
        break;
    case 1:
        r1 = x;
        g1 = c;
        break;
    case 2:
        g1 = c;
        b1 = x;
        break;
    case 3:
        g1 = x;
        b1 = c;
        break;
    case 4:
        r1 = x;
        b1 = c;
        break;
    default:
        r1 = c;
        b1 = x;
        break;
    }

    return (struct led_rgb8){
        .r = (uint8_t)(((r1 + m) * 255U) / 100U),
        .g = (uint8_t)(((g1 + m) * 255U) / 100U),
        .b = (uint8_t)(((b1 + m) * 255U) / 100U),
    };
}

static struct led_rgb8 shift_color_hue(struct led_rgb8 color, uint16_t hue_offset) {
    struct led_hsb hsb = rgb8_to_hsb(color);

    hsb.h = (uint16_t)((hsb.h + hue_offset) % 360U);
    return hsb_to_rgb8(hsb);
}

static uint16_t spectrum_hue_offset(uint32_t period_ms, uint32_t now_ms) {
    uint32_t effective_period = MAX((uint32_t)1U, period_ms);

    return (uint16_t)(((uint64_t)(now_ms % effective_period) * SPECTRUM_HUE_MAX) /
                      effective_period);
}

static uint32_t led_matrix_hash32(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

static uint32_t led_matrix_phase_offset_ms(uint16_t led, uint32_t period_ms) {
    if (period_ms == 0U) {
        return 0U;
    }

    return led_matrix_hash32((uint32_t)led + 0x9e3779b9U) % period_ms;
}

static uint8_t ease_square_pct(uint8_t pct) { return (uint8_t)(((uint16_t)pct * pct) / 100U); }

static uint8_t triangle_window_intensity(uint32_t phase_ms, uint32_t window_ms, uint8_t peak_pct) {
    if (window_ms == 0U || phase_ms >= window_ms) {
        return 0U;
    }

    uint32_t midpoint = window_ms / 2U;
    uint32_t distance = phase_ms > midpoint ? (phase_ms - midpoint) : (midpoint - phase_ms);
    uint32_t scale = midpoint == 0U ? 1U : midpoint;
    uint32_t pct = 100U - MIN(100U, (distance * 100U) / scale);

    return (uint8_t)(((uint16_t)peak_pct * pct) / 100U);
}

static uint8_t breathe_intensity(uint32_t period_ms, uint32_t now_ms) {
    uint32_t effective_period = MAX((uint32_t)2U, period_ms);
    uint32_t phase = now_ms % effective_period;
    uint32_t half = effective_period / 2U;
    uint32_t pct = half == 0U ? 100U
                              : (phase < half ? (phase * 100U) / half
                                              : ((effective_period - phase) * 100U) / half);

    return ease_square_pct((uint8_t)pct);
}

static uint8_t blink_intensity(uint32_t period_ms, uint32_t now_ms) {
    uint32_t effective_period = MAX((uint32_t)2U, period_ms);

    return (now_ms % effective_period) < (effective_period / 2U) ? 100U : 0U;
}

static uint8_t keypress_release_intensity(uint32_t fade_start_ms, uint32_t fade_deadline_ms,
                                          uint32_t now_ms) {
    if (fade_deadline_ms == 0U) {
        return 0U;
    }

    if ((int32_t)(now_ms - fade_deadline_ms) >= 0) {
        return 0U;
    }

    if ((int32_t)(now_ms - fade_start_ms) <= 0) {
        return 100U;
    }

    uint32_t fade_len = fade_deadline_ms - fade_start_ms;
    if (fade_len == 0U) {
        return 0U;
    }

    uint32_t remaining = fade_deadline_ms - now_ms;
    return (uint8_t)((remaining * 100U) / fade_len);
}

static uint8_t keypress_range_intensity(uint32_t range, uint16_t distance) {
    if (distance > range) {
        return 0U;
    }

    uint32_t span = range + 1U;
    uint32_t remaining = span - distance;

    return (uint8_t)(((remaining * remaining) * 100U) / (span * span));
}

static uint32_t ripple_step_ms_for_profile(const struct led_matrix_profile_cfg *profile) {
    uint32_t period_ms = led_matrix_effective_profile_period_ms(profile);
    uint32_t travel_cells =
        MAX((uint32_t)1U, (uint32_t)(grid_row_count + grid_col_count + RIPPLE_TAIL_CELLS));

    return MAX((uint32_t)25U, period_ms / travel_cells);
}

static inline uint16_t cell_row(uint16_t cell) { return cell_row_idx[cell]; }

static inline uint16_t cell_col(uint16_t cell) { return cell_col_idx[cell]; }

static uint16_t cell_chebyshev_distance(uint16_t lhs, uint16_t rhs) {
    uint16_t row_lhs = cell_row(lhs);
    uint16_t row_rhs = cell_row(rhs);
    uint16_t col_lhs = cell_col(lhs);
    uint16_t col_rhs = cell_col(rhs);
    uint16_t row_dist = row_lhs > row_rhs ? (row_lhs - row_rhs) : (row_rhs - row_lhs);
    uint16_t col_dist = col_lhs > col_rhs ? (col_lhs - col_rhs) : (col_rhs - col_lhs);

    return MAX(row_dist, col_dist);
}

static void clear_logical_leds(void) { memset(logical_leds, 0, sizeof(logical_leds)); }

static inline struct led_rgb8 max_blend(struct led_rgb8 a, struct led_rgb8 b) {
    return (struct led_rgb8){
        .r = MAX(a.r, b.r),
        .g = MAX(a.g, b.g),
        .b = MAX(a.b, b.b),
    };
}

#if LED_DRIVER_IS_MATRIX
static uint8_t grayscale_value(struct led_rgb8 color) {
    return MAX(color.r, MAX(color.g, color.b));
}

static int write_scalar_led_value(uint16_t led, uint8_t value, bool binary_only) {
    if (binary_only) {
        return value > 0U ? led_on(output_dev, led) : led_off(output_dev, led);
    }

    return led_set_brightness(output_dev, led, (uint8_t)(((uint16_t)value * 100U) / 255U));
}

static bool map_rgb_color_slots(const struct led_info *info, uint8_t *red_slot, uint8_t *green_slot,
                                uint8_t *blue_slot) {
    bool have_red = false;
    bool have_green = false;
    bool have_blue = false;

    if (info == NULL || info->num_colors < RGB_CHANNEL_COUNT_PER_LED ||
        info->color_mapping == NULL) {
        return false;
    }

    for (uint8_t i = 0; i < info->num_colors; i++) {
        switch (info->color_mapping[i]) {
        case LED_COLOR_ID_RED:
            *red_slot = i;
            have_red = true;
            break;
        case LED_COLOR_ID_GREEN:
            *green_slot = i;
            have_green = true;
            break;
        case LED_COLOR_ID_BLUE:
            *blue_slot = i;
            have_blue = true;
            break;
        default:
            break;
        }
    }

    return have_red && have_green && have_blue;
}
#endif

static void clear_keypress_release_state(void) {
    memset(keypress_release_fade_start_ms, 0, sizeof(keypress_release_fade_start_ms));
    memset(keypress_release_deadline_ms, 0, sizeof(keypress_release_deadline_ms));
}

static uint8_t led_matrix_dynamic_hue_index(void) {
    return led_matrix_normalize_hue_index(state.manual_hue_index) * 30U;
}

static uint32_t led_matrix_dynamic_period_ms(void) { return led_matrix_speed_scaled_period(2400U); }

static struct led_rgb8 resolve_dynamic_color(uint32_t color, size_t logical_index,
                                             uint32_t now_ms) {
    switch (color) {
    case LM_PALETTE:
        return hsb_to_rgb8(
            (struct led_hsb){.h = led_matrix_dynamic_hue_index(), .s = 100, .b = 100});
    case LM_RAINBOW: {
        uint16_t hue =
            (uint16_t)((led_matrix_dynamic_hue_index() +
                        spectrum_hue_offset(led_matrix_dynamic_period_ms(), now_ms) +
                        ((logical_index * SPECTRUM_HUE_MAX) / MAX((size_t)1U, LOGICAL_LED_COUNT))) %
                       360U);
        return hsb_to_rgb8((struct led_hsb){.h = hue, .s = 100, .b = 100});
    }
    default:
        return color24_to_rgb8(color);
    }
}

static struct led_rgb8 resolve_zone_color(const uint32_t *colors, size_t color_count,
                                          uint32_t default_color, size_t zone_index,
                                          uint32_t now_ms) {
    uint32_t color = default_color;

    if (color_count == 1U) {
        color = colors[0];
    } else if (color_count > 1U && zone_index < color_count) {
        color = colors[zone_index];
    }

    return led_matrix_color_is_dynamic(color) ? resolve_dynamic_color(color, zone_index, now_ms)
                                              : color24_to_rgb8(color);
}

static void set_zone_color_from_config(const struct led_zone *zone, const uint32_t *colors,
                                       size_t color_count, uint32_t default_color,
                                       uint32_t now_ms) {
    for (size_t i = 0; i < zone->count; i++) {
        logical_leds[zone->leds[i]] =
            resolve_zone_color(colors, color_count, default_color, i, now_ms);
    }
}

static const struct led_matrix_profile_cfg *find_display_profile(uint8_t profile_id) {
    for (size_t i = 0; i < DISPLAY_PROFILE_COUNT; i++) {
        if (display_profiles[i].profile_id == profile_id) {
            return &display_profiles[i];
        }
    }

    return NULL;
}

static const struct led_matrix_profile_cfg *selected_profile_cfg(void) {
    if (!led_matrix_has_profiles()) {
        return NULL;
    }

    return find_display_profile(state.selected_profile_id);
}

static const struct led_matrix_layer_color_cfg *
find_layer_color_cfg(zmk_keymap_layer_id_t layer_id) {
    for (size_t i = 0; i < LAYER_COLOR_COUNT; i++) {
        if (layer_colors[i].layer_id == layer_id) {
            return &layer_colors[i];
        }
    }

    return NULL;
}

static struct led_zone
led_matrix_layer_color_zone_for_cfg(const struct led_matrix_layer_color_cfg *cfg) {
    if (cfg == NULL) {
        return (struct led_zone){.leds = NULL, .count = 0U};
    }

    return cfg->has_explicit_layer_leds
               ? (struct led_zone){.leds = cfg->layer_leds, .count = cfg->layer_led_count}
               : (struct led_zone){.leds = default_layer_color_leds,
                                   .count = default_layer_color_led_count};
}

static const struct led_matrix_independent_group_cfg *
find_independent_group(uint8_t independent_id) {
    for (size_t i = 0; i < INDEPENDENT_GROUP_COUNT; i++) {
        if (independent_groups[i].independent_id == independent_id) {
            return &independent_groups[i];
        }
    }

    return NULL;
}

static const struct led_matrix_independent_profile_cfg *
find_independent_profile(const struct led_matrix_independent_group_cfg *group, uint8_t profile_id) {
    if (group == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < group->profile_count; i++) {
        if (group->profiles[i].profile_id == profile_id) {
            return &group->profiles[i];
        }
    }

    return NULL;
}

static const struct led_matrix_independent_profile_cfg *
selected_independent_profile_cfg(const struct led_matrix_independent_group_cfg *group) {
    if (group == NULL || group->independent_id >= LED_MATRIX_INDEPENDENT_PROFILE_SLOT_COUNT) {
        return NULL;
    }

    return find_independent_profile(group, selected_independent_profile_ids[group->independent_id]);
}

static size_t led_matrix_total_independent_led_count(void) {
    size_t total = 0U;

    for (size_t i = 0; i < INDEPENDENT_GROUP_COUNT; i++) {
        total += independent_groups[i].led_count;
    }

    return total;
}

static struct led_rgb8 resolve_profile_color(const struct led_matrix_profile_cfg *profile,
                                             uint16_t led, uint32_t now_ms) {
    uint32_t color = LM_BLACK;

    if (profile == NULL || profile->color_map_count == 0U) {
        return (struct led_rgb8){0, 0, 0};
    }

    if (profile->color_map_count == 1U) {
        color = profile->color_map[0];
    } else if (led < profile->color_map_count) {
        color = profile->color_map[led];
    }

    return led_matrix_color_is_dynamic(color) ? resolve_dynamic_color(color, led, now_ms)
                                              : color24_to_rgb8(color);
}

static struct led_rgb8
resolve_independent_profile_color(const struct led_matrix_independent_profile_cfg *profile,
                                  size_t index, uint32_t now_ms) {
    uint32_t color = LM_BLACK;

    if (profile == NULL || profile->color_map_count == 0U) {
        return (struct led_rgb8){0, 0, 0};
    }

    if (profile->color_map_count == 1U) {
        color = profile->color_map[0];
    } else if (index < profile->color_map_count) {
        color = profile->color_map[index];
    }

    return led_matrix_color_is_dynamic(color) ? resolve_dynamic_color(color, index, now_ms)
                                              : color24_to_rgb8(color);
}

static struct led_rgb8 resolve_battery_on_color(uint8_t soc, size_t zone_index, uint32_t now_ms) {
    if (battery_level_color_cfg.count > 0U) {
        size_t bucket = MIN(battery_level_color_cfg.count - 1U,
                            ((size_t)soc * battery_level_color_cfg.count) / 101U);
        return resolve_zone_color(battery_level_color_cfg.colors, battery_level_color_cfg.count,
                                  battery_level_color_cfg.default_color, bucket, now_ms);
    }

    return resolve_zone_color(battery_color_on_cfg.colors, battery_color_on_cfg.count,
                              battery_color_on_cfg.default_color, zone_index, now_ms);
}

static const struct led_matrix_color_list_cfg *
led_matrix_usb_color_cfg_for_state(enum zmk_usb_conn_state usb_state) {
    switch (usb_state) {
    case ZMK_USB_CONN_HID:
        return &usb_color_hid_cfg;
    case ZMK_USB_CONN_POWERED:
        return &usb_color_powered_cfg;
    case ZMK_USB_CONN_NONE:
    default:
        return &usb_color_none_cfg;
    }
}

static const struct led_matrix_color_list_cfg *
led_matrix_ble_color_cfg_for_snapshot(const struct led_matrix_snapshot *snapshot) {
    if (snapshot != NULL && snapshot->ble_open) {
        return &ble_color_open_cfg;
    }

    if (snapshot != NULL && snapshot->ble_connected) {
        return &ble_color_connected_cfg;
    }

    return &ble_color_disconnected_cfg;
}

static const struct led_matrix_status_source_descriptor *
led_matrix_status_source_descriptor_for_source(enum led_matrix_status_source source);
static const struct led_matrix_status_source_descriptor status_source_descriptors[4];
static const struct led_matrix_hid_indicator_descriptor hid_indicator_descriptors[5];
extern struct k_work_delayable led_matrix_clear_status_work;
extern struct k_work_delayable led_matrix_clear_layer_color_work;
extern struct k_work led_matrix_render_work;
extern struct k_timer led_matrix_tick;
static void led_matrix_invalidate_flushed_frame(void);
static int led_matrix_flush_cleared_output(void);
static int led_matrix_raise_state_changed(uint8_t reason_mask);
static void led_matrix_queue_service_work(uint32_t requests);
static int led_matrix_reschedule_delayable_work(struct k_work_delayable *work, k_timeout_t delay);
static void led_matrix_sync_tick_state(void);
static void led_matrix_schedule_render(void);
static uint32_t
timeout_for_sources_with_wake_context(uint8_t sources,
                                      enum led_matrix_status_wake_context wake_context);
static inline bool led_matrix_base_render_enabled(void);
static inline bool led_matrix_render_enabled(void);
static uint8_t led_matrix_current_battery_soc(void);
static enum zmk_usb_conn_state led_matrix_current_usb_state(void);
static uint8_t led_matrix_current_ble_profile(void);
static bool led_matrix_current_ble_connected(void);
static bool led_matrix_current_ble_open(void);
static zmk_hid_indicators_t led_matrix_current_hid_indicators(void);
static enum zmk_transport led_matrix_current_output_transport(void);
static void render_battery_overlay(uint32_t now_ms);
static void render_usb_overlay(uint32_t now_ms);
static void render_ble_overlay(uint32_t now_ms);
static void render_hid_overlay(uint32_t now_ms);

static uint8_t led_matrix_filter_available_status_sources(uint8_t sources) {
    uint8_t filtered = 0U;

    for (size_t i = 0; i < ARRAY_SIZE(status_source_descriptors); i++) {
        const struct led_matrix_status_source_descriptor *cfg = &status_source_descriptors[i];

        if ((sources & cfg->source) == 0U || !cfg->has_zone() || !cfg->snapshot_valid(&snapshot)) {
            continue;
        }

        filtered |= cfg->source;
    }

    return filtered;
}

static uint8_t led_matrix_filter_status_sources(uint8_t sources, bool persistent_only) {
    uint8_t filtered = 0U;
    uint8_t available = led_matrix_filter_available_status_sources(sources);

    for (size_t i = 0; i < ARRAY_SIZE(status_source_descriptors); i++) {
        const struct led_matrix_status_source_descriptor *cfg = &status_source_descriptors[i];
        bool persistent = cfg->timeout_ms == 0U;

        if ((available & cfg->source) == 0U || persistent != persistent_only) {
            continue;
        }

        filtered |= cfg->source;
    }

    return filtered;
}

static void led_matrix_refresh_automatic_persistent_status_sources(void) {
    uint8_t mask = 0U;

    for (size_t i = 0; i < ARRAY_SIZE(status_source_descriptors); i++) {
        const struct led_matrix_status_source_descriptor *cfg = &status_source_descriptors[i];

        if (cfg->timeout_ms == 0U && cfg->has_zone() && cfg->snapshot_valid(&snapshot)) {
            mask |= cfg->source;
        }
    }

    automatic_persistent_status_sources_mask = mask;
}

static uint8_t current_status_sources(void) {
    uint8_t sources = 0U;

    if (led_matrix_base_render_enabled()) {
        sources |= automatic_persistent_status_sources_mask;
    }

    if (status_mode == STATUS_MODE_TRANSIENT) {
        sources |= transient_sources;
    }

    return led_matrix_filter_available_status_sources(sources);
}

static bool led_matrix_status_needs_periodic_refresh(uint32_t now_ms) {
    uint8_t sources = current_status_sources();

    for (size_t i = 0; i < ARRAY_SIZE(status_source_descriptors); i++) {
        const struct led_matrix_status_source_descriptor *cfg = &status_source_descriptors[i];

        if ((sources & cfg->source) != 0U && cfg->needs_periodic_refresh(now_ms)) {
            return true;
        }
    }

    return false;
}

static bool
led_matrix_profile_has_time_dynamic_color(const struct led_matrix_profile_cfg *profile) {
    return profile != NULL && led_matrix_color_list_has_time_dynamic_color(
                                  profile->color_map, profile->color_map_count);
}

static bool led_matrix_independent_profile_has_time_dynamic_color(
    const struct led_matrix_independent_profile_cfg *profile) {
    return profile != NULL && led_matrix_color_list_has_time_dynamic_color(
                                  profile->color_map, profile->color_map_count);
}

static bool led_matrix_profile_needs_periodic_refresh(const struct led_matrix_profile_cfg *profile,
                                                      uint32_t now_ms) {
    ARG_UNUSED(now_ms);

    if (profile == NULL) {
        return false;
    }

    if (led_matrix_profile_has_time_dynamic_color(profile)) {
        return true;
    }

    switch (profile->renderer) {
    case LED_MATRIX_RENDERER_BREATHE:
    case LED_MATRIX_RENDERER_SPECTRUM:
    case LED_MATRIX_RENDERER_SWIRL:
    case LED_MATRIX_RENDERER_RAIN:
    case LED_MATRIX_RENDERER_STARLIGHT:
        return true;
    case LED_MATRIX_RENDERER_KEYPRESS:
        for (size_t i = 0; i < ZMK_KEYMAP_LEN; i++) {
            if (pressed_positions[i] || keypress_release_deadline_ms[i] != 0U) {
                return true;
            }
        }
        return false;
    case LED_MATRIX_RENDERER_RIPPLE:
        return ripple_state.active;
    case LED_MATRIX_RENDERER_SOLID:
    default:
        return false;
    }
}

static bool led_matrix_independent_groups_need_periodic_refresh(uint32_t now_ms) {
    ARG_UNUSED(now_ms);

    for (size_t i = 0; i < INDEPENDENT_GROUP_COUNT; i++) {
        const struct led_matrix_independent_profile_cfg *profile =
            selected_independent_profile_cfg(&independent_groups[i]);

        if (profile == NULL) {
            continue;
        }

        if (led_matrix_independent_profile_has_time_dynamic_color(profile)) {
            return true;
        }

        if (profile->renderer == LED_MATRIX_RENDERER_BREATHE ||
            profile->renderer == LED_MATRIX_RENDERER_SPECTRUM ||
            profile->renderer == LED_MATRIX_RENDERER_SWIRL) {
            return true;
        }
    }

    return false;
}

static bool led_matrix_layer_color_needs_periodic_refresh(void) {
    if (!layer_color.active || layer_color.cfg == NULL) {
        return false;
    }

    return led_matrix_color_list_has_time_dynamic_color(layer_color.cfg->layer_colors,
                                                        layer_color.cfg->layer_color_count);
}

static bool led_matrix_battery_status_needs_periodic_refresh(uint32_t now_ms) {
    ARG_UNUSED(now_ms);

    return led_matrix_color_list_has_time_dynamic_color(battery_color_on_cfg.colors,
                                                        battery_color_on_cfg.count) ||
           led_matrix_color_list_has_time_dynamic_color(battery_color_off_cfg.colors,
                                                        battery_color_off_cfg.count) ||
           led_matrix_color_list_has_time_dynamic_color(battery_level_color_cfg.colors,
                                                        battery_level_color_cfg.count);
}

static bool led_matrix_usb_status_needs_periodic_refresh(uint32_t now_ms) {
    ARG_UNUSED(now_ms);

    return led_matrix_color_list_has_time_dynamic_color(usb_color_hid_cfg.colors,
                                                        usb_color_hid_cfg.count) ||
           led_matrix_color_list_has_time_dynamic_color(usb_color_powered_cfg.colors,
                                                        usb_color_powered_cfg.count) ||
           led_matrix_color_list_has_time_dynamic_color(usb_color_none_cfg.colors,
                                                        usb_color_none_cfg.count);
}

static bool led_matrix_ble_status_needs_periodic_refresh(uint32_t now_ms) {
    ARG_UNUSED(now_ms);

    return ble_pairing_transient_active ||
           led_matrix_color_list_has_time_dynamic_color(ble_color_open_cfg.colors,
                                                        ble_color_open_cfg.count) ||
           led_matrix_color_list_has_time_dynamic_color(ble_color_connected_cfg.colors,
                                                        ble_color_connected_cfg.count) ||
           led_matrix_color_list_has_time_dynamic_color(ble_color_disconnected_cfg.colors,
                                                        ble_color_disconnected_cfg.count);
}

static bool led_matrix_hid_indicator_needs_periodic_refresh(
    const struct led_matrix_hid_indicator_descriptor *descriptor, uint32_t now_ms) {
    ARG_UNUSED(now_ms);

    return descriptor != NULL && (led_matrix_color_list_has_time_dynamic_color(
                                      descriptor->color_on->colors, descriptor->color_on->count) ||
                                  led_matrix_color_list_has_time_dynamic_color(
                                      descriptor->color_off->colors, descriptor->color_off->count));
}

static bool led_matrix_hid_status_needs_periodic_refresh(uint32_t now_ms) {
    for (size_t i = 0; i < ARRAY_SIZE(hid_indicator_descriptors); i++) {
        if (led_matrix_hid_indicator_needs_periodic_refresh(&hid_indicator_descriptors[i],
                                                            now_ms)) {
            return true;
        }
    }

    return false;
}

static bool led_matrix_wake_context_requires_transient_status(
    enum led_matrix_status_wake_context wake_context) {
    return wake_context != LED_MATRIX_WAKE_CONTEXT_NONE;
}

static enum led_matrix_status_wake_context led_matrix_resolve_status_wake_context(void) {
    if (sleep_suspended) {
        return LED_MATRIX_WAKE_CONTEXT_NONE;
    }

#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
    if (idle_auto_off_state.suspended) {
        return LED_MATRIX_WAKE_CONTEXT_IDLE_AUTO_OFF_STATUS_ONLY;
    }
#endif

    if (!state.on) {
        return LED_MATRIX_WAKE_CONTEXT_MANUAL_OFF_TRANSIENT;
    }

    return LED_MATRIX_WAKE_CONTEXT_NONE;
}

#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
static bool led_matrix_usb_powered(void) {
    return led_matrix_current_usb_state() != ZMK_USB_CONN_NONE;
}
#endif

static void render_hid_indicator(const struct led_matrix_hid_indicator_descriptor *descriptor,
                                 uint32_t now_ms) {
    if (descriptor == NULL || descriptor->zone->count == 0U) {
        return;
    }

    bool active = (snapshot.hid & descriptor->bit) != 0U;

    set_zone_color_from_config(
        descriptor->zone, active ? descriptor->color_on->colors : descriptor->color_off->colors,
        active ? descriptor->color_on->count : descriptor->color_off->count,
        active ? descriptor->color_on->default_color : descriptor->color_off->default_color,
        now_ms);
}

static void led_matrix_reset_rain_state(void) { memset(&rain_state, 0, sizeof(rain_state)); }

static void rain_sync_state(uint32_t current_step, uint32_t step_ms) {
    if (grid_col_count == 0U) {
        led_matrix_reset_rain_state();
        return;
    }

    if (!rain_state.initialized || rain_state.step_ms != step_ms ||
        current_step < rain_state.last_step) {
        led_matrix_reset_rain_state();
        rain_state.initialized = true;
        rain_state.step_ms = step_ms;
        rain_state.last_step = current_step;
    }

    while (rain_state.last_step < current_step) {
        rain_state.last_step++;

        for (size_t col = 0; col < grid_col_count; col++) {
            struct led_matrix_rain_column_state *column = &rain_state.columns[col];

            if (column->active) {
                column->age_x100 += column->speed_pct;
                if ((column->age_x100 / 100U) >
                    (uint32_t)(grid_row_count + column->streak_cells + RAIN_TAIL_CELLS)) {
                    column->active = false;
                    column->streak_cells = 0U;
                    column->speed_pct = 0U;
                    column->age_x100 = 0U;
                }
                continue;
            }

            uint32_t hash = led_matrix_hash32(rain_state.last_step + (uint32_t)(col * 97U));
            if ((hash % MAX((uint32_t)2U, grid_col_count + 1U)) != 0U) {
                continue;
            }

            column->active = true;
            column->streak_cells = (uint8_t)(1U + (hash % 3U));
            uint32_t speed_hash = led_matrix_hash32(hash ^ 0x8A5CD789U);
            column->speed_pct =
                (uint8_t)(RAIN_MIN_SPEED_PCT +
                          (speed_hash % (RAIN_MAX_SPEED_PCT - RAIN_MIN_SPEED_PCT + 1U)));
            column->age_x100 = 0U;
        }
    }
}

static void led_matrix_refresh_status_snapshot(struct led_matrix_snapshot *out) {
    if (out == NULL) {
        return;
    }

    out->battery_soc = led_matrix_current_battery_soc();
    out->usb_state = led_matrix_current_usb_state();
    out->ble_profile = led_matrix_current_ble_profile();
    out->ble_connected = led_matrix_current_ble_connected();
    out->ble_open = led_matrix_current_ble_open();
    out->hid = led_matrix_current_hid_indicators();
    out->output_transport = led_matrix_current_output_transport();
}

static uint8_t led_matrix_current_battery_soc(void) {
#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    return zmk_battery_state_of_charge();
#else
    return 100U;
#endif
}

static enum zmk_usb_conn_state led_matrix_current_usb_state(void) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    return zmk_usb_get_conn_state();
#else
    return ZMK_USB_CONN_NONE;
#endif
}

static uint8_t led_matrix_current_ble_profile(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE) && LED_MATRIX_HAS_CENTRAL_STATE
    return (uint8_t)zmk_ble_active_profile_index();
#else
    return 0U;
#endif
}

static bool led_matrix_current_ble_connected(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE) && LED_MATRIX_HAS_CENTRAL_STATE
    return zmk_ble_active_profile_is_connected();
#else
    return false;
#endif
}

static bool led_matrix_current_ble_open(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE) && LED_MATRIX_HAS_CENTRAL_STATE
    return zmk_ble_active_profile_is_open();
#else
    return false;
#endif
}

static zmk_hid_indicators_t led_matrix_current_hid_indicators(void) {
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS) && LED_MATRIX_HAS_CENTRAL_STATE
    return zmk_hid_indicators_get_current_profile();
#else
    return 0U;
#endif
}

static enum zmk_transport led_matrix_current_output_transport(void) {
#if LED_MATRIX_HAS_CENTRAL_STATE
    struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();

    if (selected.transport != ZMK_TRANSPORT_NONE) {
        return selected.transport;
    }

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    if (zmk_endpoint_get_preferred_transport() == ZMK_TRANSPORT_USB && zmk_usb_is_hid_ready()) {
        return ZMK_TRANSPORT_USB;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
    return ZMK_TRANSPORT_BLE;
#else
    return ZMK_TRANSPORT_NONE;
#endif
#else
    return ZMK_TRANSPORT_NONE;
#endif
}

static const struct led_matrix_status_source_descriptor status_source_descriptors[] = {
    {
        .source = LED_MATRIX_STATUS_BATTERY,
        .name = "battery",
        .timeout_ms = BATTERY_TIMEOUT_MS,
        .has_zone = led_matrix_status_source_has_battery_zone,
        .snapshot_valid = led_matrix_battery_snapshot_valid,
        .needs_periodic_refresh = led_matrix_battery_status_needs_periodic_refresh,
        .render_overlay = render_battery_overlay,
    },
    {
        .source = LED_MATRIX_STATUS_USB,
        .name = "usb",
        .timeout_ms = USB_TIMEOUT_MS,
        .has_zone = led_matrix_status_source_has_usb_zone,
        .snapshot_valid = led_matrix_usb_snapshot_valid,
        .needs_periodic_refresh = led_matrix_usb_status_needs_periodic_refresh,
        .render_overlay = render_usb_overlay,
    },
    {
        .source = LED_MATRIX_STATUS_BLE,
        .name = "ble",
        .timeout_ms = BLE_TIMEOUT_MS,
        .has_zone = led_matrix_status_source_has_ble_zone,
        .snapshot_valid = led_matrix_ble_snapshot_valid,
        .needs_periodic_refresh = led_matrix_ble_status_needs_periodic_refresh,
        .render_overlay = render_ble_overlay,
    },
    {
        .source = LED_MATRIX_STATUS_HID,
        .name = "hid",
        .timeout_ms = HID_TIMEOUT_MS,
        .has_zone = led_matrix_status_source_has_hid_zone,
        .snapshot_valid = led_matrix_hid_snapshot_valid,
        .needs_periodic_refresh = led_matrix_hid_status_needs_periodic_refresh,
        .render_overlay = render_hid_overlay,
    },
};

static const struct led_matrix_hid_indicator_descriptor hid_indicator_descriptors[] = {
    {
        .zone_name = "hid-caps-leds",
        .color_on_name = "hid-caps-color-on",
        .color_off_name = "hid-caps-color-off",
        .zone = &hid_caps_zone,
        .color_on = &hid_caps_color_on_cfg,
        .color_off = &hid_caps_color_off_cfg,
        .bit = HID_LED_CAPSLOCK_BIT,
    },
    {
        .zone_name = "hid-num-leds",
        .color_on_name = "hid-num-color-on",
        .color_off_name = "hid-num-color-off",
        .zone = &hid_num_zone,
        .color_on = &hid_num_color_on_cfg,
        .color_off = &hid_num_color_off_cfg,
        .bit = HID_LED_NUMLOCK_BIT,
    },
    {
        .zone_name = "hid-scroll-leds",
        .color_on_name = "hid-scroll-color-on",
        .color_off_name = "hid-scroll-color-off",
        .zone = &hid_scroll_zone,
        .color_on = &hid_scroll_color_on_cfg,
        .color_off = &hid_scroll_color_off_cfg,
        .bit = HID_LED_SCROLLLOCK_BIT,
    },
    {
        .zone_name = "hid-compose-leds",
        .color_on_name = "hid-compose-color-on",
        .color_off_name = "hid-compose-color-off",
        .zone = &hid_compose_zone,
        .color_on = &hid_compose_color_on_cfg,
        .color_off = &hid_compose_color_off_cfg,
        .bit = HID_LED_COMPOSE_BIT,
    },
    {
        .zone_name = "hid-kana-leds",
        .color_on_name = "hid-kana-color-on",
        .color_off_name = "hid-kana-color-off",
        .zone = &hid_kana_zone,
        .color_on = &hid_kana_color_on_cfg,
        .color_off = &hid_kana_color_off_cfg,
        .bit = HID_LED_KANA_BIT,
    },
};

static const struct led_matrix_zone_validation_descriptor status_zone_validation_descriptors[] = {
    {.name = "battery-leds", .zone = &battery_zone},
    {.name = "ble-profile-leds", .zone = &ble_zone},
    {.name = "usb-leds", .zone = &usb_zone},
    {.name = "hid-caps-leds", .zone = &hid_caps_zone},
    {.name = "hid-num-leds", .zone = &hid_num_zone},
    {.name = "hid-scroll-leds", .zone = &hid_scroll_zone},
    {.name = "hid-compose-leds", .zone = &hid_compose_zone},
    {.name = "hid-kana-leds", .zone = &hid_kana_zone},
};

static const struct led_matrix_color_validation_descriptor status_color_validation_descriptors[] = {
    {.name = "battery-color-on", .colors = &battery_color_on_cfg, .expected_zone = &battery_zone},
    {.name = "battery-color-off", .colors = &battery_color_off_cfg, .expected_zone = &battery_zone},
    {.name = "battery-level-colors",
     .colors = &battery_level_color_cfg,
     .expected_zone = NULL,
     .bucket_colors = true},
    {.name = "usb-color-hid", .colors = &usb_color_hid_cfg, .expected_zone = &usb_zone},
    {.name = "usb-color-powered", .colors = &usb_color_powered_cfg, .expected_zone = &usb_zone},
    {.name = "usb-color-none", .colors = &usb_color_none_cfg, .expected_zone = &usb_zone},
    {.name = "ble-color-open", .colors = &ble_color_open_cfg, .expected_zone = &ble_zone},
    {.name = "ble-color-connected", .colors = &ble_color_connected_cfg, .expected_zone = &ble_zone},
    {.name = "ble-color-disconnected",
     .colors = &ble_color_disconnected_cfg,
     .expected_zone = &ble_zone},
};

static const struct led_matrix_status_source_descriptor *
led_matrix_status_source_descriptor_for_source(enum led_matrix_status_source source) {
    for (size_t i = 0; i < ARRAY_SIZE(status_source_descriptors); i++) {
        if (status_source_descriptors[i].source == source) {
            return &status_source_descriptors[i];
        }
    }

    return NULL;
}
