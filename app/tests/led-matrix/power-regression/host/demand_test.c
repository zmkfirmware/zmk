/* SPDX-License-Identifier: MIT */
#include "demand_fakes.h"
#include "demand_math.inc"
#include "demand_core.inc"
#include "src/led_matrix/render_demand.c"
#include "demand_render.inc"
#include "demand_compose.inc"
#include "position.inc"

static unsigned checks;
static struct led_matrix_color_list_cfg color_cfg(uint32_t *color) {
    return (struct led_matrix_color_list_cfg){
        .colors = color, .count = 1, .default_color = LM_BLACK};
}
static void reset(void) {
    test_now_ms = 0;
    scheduled_renders = 0;
    state.on = true;
    state.brightness = 100;
    force_show_active = sleeping = idle_suspended = independent_present = false;
    profile_present = true;
    ble_pairing_transient_active = false;
    source_mask = 0;
    snapshot = (struct led_matrix_snapshot){0};
    memset(logical_leds, 0, sizeof(logical_leds));
    memset(logical_leds_dynamic, 0, sizeof(logical_leds_dynamic));
    memset(pressed_positions, 0, sizeof(pressed_positions));
    memset(keypress_release_fade_start_ms, 0, sizeof(keypress_release_fade_start_ms));
    memset(keypress_release_deadline_ms, 0, sizeof(keypress_release_deadline_ms));
    memset(&ripple_state, 0, sizeof(ripple_state));
    memset(&rain_state, 0, sizeof(rain_state));
    memset(base_colors, 0, sizeof(base_colors));
    memset(independent_colors, 0, sizeof(independent_colors));
    memset(layer_colors, 0, sizeof(layer_colors));
    memset(usb_colors, 0, sizeof(usb_colors));
    memset(ble_colors, 0, sizeof(ble_colors));
    memset(battery_colors, 0, sizeof(battery_colors));
    profile = (struct led_matrix_profile_cfg){.renderer = LED_MATRIX_RENDERER_SOLID,
                                              .period_ms = 1000,
                                              .brightness = 100,
                                              .color_map = base_colors,
                                              .color_map_count = 1};
    independent_profile =
        (struct led_matrix_independent_profile_cfg){.renderer = LED_MATRIX_RENDERER_SOLID,
                                                    .period_ms = 1000,
                                                    .brightness = 100,
                                                    .color_map = independent_colors,
                                                    .color_map_count = 1};
    independent_groups[0] =
        (struct led_matrix_independent_group_cfg){.leds = all_leds, .led_count = 4};
    layer_cfg = (struct led_matrix_layer_color_cfg){.brightness = 100,
                                                    .layer_colors = layer_colors,
                                                    .layer_color_count = 1,
                                                    .layer_leds = all_leds,
                                                    .layer_led_count = 4};
    layer_color.active = false;
    layer_color.cfg = &layer_cfg;
    battery_zone = (struct led_zone){.leds = all_leds, .count = 0};
    usb_zone = ble_zone = battery_zone;
    battery_color_on_cfg = color_cfg(&battery_colors[0]);
    battery_color_off_cfg = color_cfg(&battery_colors[1]);
    battery_level_color_cfg = (struct led_matrix_color_list_cfg){0};
    usb_color_none_cfg = color_cfg(&usb_colors[0]);
    usb_color_powered_cfg = color_cfg(&usb_colors[1]);
    usb_color_hid_cfg = color_cfg(&usb_colors[2]);
    ble_color_open_cfg = color_cfg(&ble_colors[0]);
    ble_color_connected_cfg = color_cfg(&ble_colors[1]);
    ble_color_disconnected_cfg = color_cfg(&ble_colors[2]);
    for (size_t i = 0; i < 5; i++) {
        hid_zones[i] = (struct led_zone){.leds = all_leds, .count = 0};
        on_colors[i] = LM_WHITE;
        off_colors[i] = LM_BLACK;
        hid_on_cfg[i] = color_cfg(&on_colors[i]);
        hid_off_cfg[i] = color_cfg(&off_colors[i]);
        hid_indicator_descriptors[i] =
            (struct led_matrix_hid_indicator_descriptor){.zone = &hid_zones[i],
                                                         .color_on = &hid_on_cfg[i],
                                                         .color_off = &hid_off_cfg[i],
                                                         .bit = BIT(i)};
    }
    hid_indicator_descriptors[0].bit = BIT(1);
    hid_indicator_descriptors[1].bit = BIT(0);
    matrix_output_sink.kind = LED_MATRIX_MATRIX_SINK_CHANNELS;
}
static void expect(const char *label, uint32_t now, bool nonzero, bool periodic) {
    bool on = state.on;
    uint8_t brightness = state.brightness;
    struct led_matrix_render_demand demand = compose_render_frame(now);
    if (demand.nonzero != nonzero || demand.periodic != periodic) {
        fprintf(stderr, "%s at %u: got {%d,%d}, expected {%d,%d}\n", label, now, demand.nonzero,
                demand.periodic, nonzero, periodic);
        abort();
    }
    assert(state.on == on && state.brightness == brightness);
    checks++;
}
static void static_and_animation(void) {
    reset();
    expect("static black", 0, false, false);
    base_colors[0] = LM_WHITE;
    expect("static white", 0, true, false);
    base_colors[0] = LM_RAINBOW;
    expect("solid rainbow", 0, true, true);
    state.brightness = 0;
    expect("global brightness zero", 0, false, false);
    state.brightness = 100;
    profile.brightness = 0;
    expect("profile brightness zero", 0, false, false);
    profile.brightness = 100;
    profile.renderer = LED_MATRIX_RENDERER_BREATHE;
    expect("breathe black phase must stay awake", 0, false, true);
    expect("breathe visible phase", 500, true, true);
    base_colors[0] = LM_BLACK;
    expect("breathe permanently black", 500, false, false);
    base_colors[0] = LM_PALETTE;
    profile.renderer = LED_MATRIX_RENDERER_SOLID;
    expect("palette is event driven", 0, true, false);
    base_colors[0] = LM_RGB(1, 0, 0);
    profile.renderer = LED_MATRIX_RENDERER_BREATHE;
    matrix_output_sink.kind = LED_MATRIX_MATRIX_SINK_SCALAR_LED;
    expect("scalar brightness quantizes permanently dark", 500, false, false);
    matrix_output_sink.kind = LED_MATRIX_MATRIX_SINK_CHANNELS;
    profile.renderer = LED_MATRIX_RENDERER_SPECTRUM;
    expect("hue conversion quantizes faint color to permanent black", 500, false, false);
    base_colors[0] = LM_WHITE;
    expect("rotating gray hue is static", 500, true, false);
}
static void status_selection(void) {
    reset();
    source_mask = LED_MATRIX_STATUS_HID;
    hid_zones[0].count = 1;
    on_colors[0] = LM_RAINBOW;
    expect("Caps off ignores dynamic on", 0, false, false);
    snapshot.hid = BIT(1);
    expect("Caps on rainbow", 0, true, true);
    snapshot.hid = 0;
    on_colors[1] = LM_RAINBOW;
    expect("unconfigured Num zone cannot animate", 0, false, false);
    reset();
    source_mask = LED_MATRIX_STATUS_USB;
    usb_zone.count = 1;
    usb_colors[2] = LM_RAINBOW;
    expect("USB none ignores HID color", 0, false, false);
    snapshot.usb_state = ZMK_USB_CONN_HID;
    expect("USB selected rainbow", 0, true, true);
    reset();
    source_mask = LED_MATRIX_STATUS_BLE;
    ble_zone.count = 4;
    snapshot.ble_connected = true;
    ble_colors[0] = LM_RAINBOW;
    expect("BLE connected ignores open color", 0, false, false);
    snapshot.ble_open = true;
    expect("BLE open rainbow", 0, true, true);
    ble_pairing_transient_active = true;
    expect("pairing black blink phase retains future demand", 400, false, true);
    ble_colors[0] = LM_BLACK;
    expect("pairing black configured color", 400, false, false);
    snapshot.ble_profile = 4;
    expect("out of range BLE profile", 0, false, false);
    reset();
    source_mask = LED_MATRIX_STATUS_BATTERY;
    battery_zone.count = 4;
    battery_colors[0] = LM_RAINBOW;
    expect("empty battery ignores on color", 0, false, false);
    snapshot.battery_soc = 100;
    expect("full battery selected on", 0, true, true);
    battery_colors[0] = LM_WHITE;
    battery_colors[1] = LM_RAINBOW;
    expect("full battery ignores off color", 0, true, false);
    uint32_t buckets[] = {LM_WHITE, LM_RAINBOW};
    battery_level_color_cfg = (struct led_matrix_color_list_cfg){.colors = buckets, .count = 2};
    battery_colors[1] = LM_BLACK;
    snapshot.battery_soc = 1;
    expect("unused battery rainbow bucket does not refresh", 0, true, false);
    snapshot.battery_soc = 100;
    expect("battery selects rainbow bucket", 0, true, true);
}
static void overwrite_priority(void) {
    reset();
    base_colors[0] = LM_RAINBOW;
    source_mask = LED_MATRIX_STATUS_HID;
    hid_zones[0].count = 4;
    expect("black HID covers all base animation", 0, false, false);
    hid_zones[0].count = 3;
    expect("uncovered base LED keeps animation", 0, true, true);
    hid_zones[0].count = 4;
    snapshot.hid = BIT(1);
    on_colors[0] = LM_RAINBOW;
    hid_zones[1].count = 4;
    expect("later inactive Num hides active Caps rainbow", 0, false, false);
    hid_zones[1].count = 0;
    expect("removing covering Num reveals Caps", 0, true, true);
    source_mask = 0;
    independent_present = true;
    expect("independent black overwrites base flags", 0, false, false);
    independent_groups[0].led_count = 3;
    expect("partial independent keeps base", 0, true, true);
    layer_color.active = true;
    expect("layer black overwrites every source", 0, false, false);
    layer_color.active = false;
    expect("layer ending restores underlying animation", 0, true, true);
    reset();
    base_colors[0] = LM_RAINBOW;
    source_mask = LED_MATRIX_STATUS_BLE;
    ble_zone.count = 4;
    ble_colors[2] = LM_WHITE;
    snapshot.ble_profile = 0;
    expect("BLE clears entire zone including dynamic flags", 0, true, false);
    source_mask |= LED_MATRIX_STATUS_USB;
    usb_zone.count = 4;
    usb_colors[0] = LM_RAINBOW;
    expect("BLE later static overrides USB rainbow", 0, true, false);
}
static void keypress_and_ripple(void) {
    reset();
    profile.renderer = LED_MATRIX_RENDERER_KEYPRESS;
    base_colors[0] = LM_RAINBOW;
    expect("keypress has no autonomous rainbow without keys", 0, false, false);
    pressed_positions[0] = true;
    expect("held rainbow key", 0, true, true);
    base_colors[0] = LM_WHITE;
    expect("held static key needs no timer", 0, true, false);
    pressed_positions[0] = false;
    keypress_release_fade_start_ms[0] = 100;
    keypress_release_deadline_ms[0] = 200;
    expect("release afterglow awaits fade", 50, true, true);
    expect("release fade", 150, true, true);
    expect("release expires", 200, false, false);
    assert(keypress_release_deadline_ms[0] == 0 && keypress_release_fade_start_ms[0] == 0);
    pressed_positions[0] = true;
    expect("next press restores static output", 300, true, false);
    pressed_positions[0] = false;
    keypress_release_fade_start_ms[0] = UINT32_MAX - 99U;
    keypress_release_deadline_ms[0] = 100;
    expect("release handles uptime wrap", UINT32_MAX - 49U, true, true);
    expect("wrapped release expires", 100, false, false);
    reset();
    profile.renderer = LED_MATRIX_RENDERER_RIPPLE;
    base_colors[0] = LM_RAINBOW;
    expect("inactive ripple ignores rainbow", 0, false, false);
    ripple_state.active = true;
    ripple_state.start_time_ms = 0;
    ripple_state.origin_cell = 0;
    expect("triggered ripple", 0, true, true);
    expect("ripple expiration", 10000, false, false);
    assert(!ripple_state.active);
    ripple_state.active = true;
    ripple_state.start_time_ms = 10000;
    expect("next trigger restores ripple", 10000, true, true);
}
static void runtime_gates(void) {
    reset();
    base_colors[0] = LM_RAINBOW;
    state.on = false;
    expect("manual off", 0, false, false);
    source_mask = LED_MATRIX_STATUS_HID;
    hid_zones[0].count = 1;
    snapshot.hid = BIT(1);
    on_colors[0] = LM_RAINBOW;
    force_show_active = true;
    expect("off transient status can wake", 0, true, true);
    force_show_active = false;
    expect("off transient ends", 1, false, false);
    state.on = true;
    idle_suspended = true;
    expect("idle keeps logical on but no base", 0, false, false);
    force_show_active = true;
    expect("idle transient status can wake", 0, true, true);
    sleeping = true;
    force_show_active = false;
    expect("sleep stopped", 0, false, false);
}
static void rain_resume_is_bounded(void) {
    reset();
    rain_sync_state(100U, 50U);
    rain_state.columns[0] = (struct led_matrix_rain_column_state){
        .active = true, .streak_cells = 3, .speed_pct = 100, .age_x100 = 100};
    rain_sync_state(101U, 50U);
    assert(rain_state.last_step == 101U && rain_state.columns[0].active &&
           rain_state.columns[0].age_x100 == 200U);
    /* A huge hidden gap must resynchronize, not iterate four billion steps. */
    rain_sync_state(UINT32_MAX - 1U, 50U);
    assert(rain_state.last_step == UINT32_MAX - 1U);
    for (size_t i = 0; i < grid_col_count; i++)
        assert(!rain_state.columns[i].active);
    profile.renderer = LED_MATRIX_RENDERER_RAIN;
    base_colors[0] = LM_WHITE;
    source_mask = LED_MATRIX_STATUS_HID;
    hid_zones[0].count = 4;
    expect("hidden rain has no visible timer demand", 0, false, false);
    source_mask = 0;
    expect("rain revealed after a day restarts without history replay", 86400000U, false, true);
}
static void position_event_lifecycle(void) {
    reset();
    profile.renderer = LED_MATRIX_RENDERER_KEYPRESS;
    profile.keypress_afterglow_ms = 100;
    profile.keypress_fadeout_ms = 100;
    base_colors[0] = LM_WHITE;
    idle_suspended = true;
    test_now_ms = 100;
    led_matrix_handle_position_event(0, true);
    assert(!idle_suspended && pressed_positions[0] && scheduled_renders == 1);
    expect("idle first key is retained and rendered", test_now_ms, true, false);
    led_matrix_handle_position_event(0, false);
    assert(!pressed_positions[0] && keypress_release_deadline_ms[0] == 300);
    expect("event creates release afterglow", test_now_ms, true, true);
    expect("event release expires normally", 300, false, false);
    test_now_ms = 400;
    led_matrix_handle_position_event(0, true);
    expect("next real key event restores output", test_now_ms, true, false);
    state.on = false;
    unsigned scheduled_before_release = scheduled_renders;
    led_matrix_handle_position_event(0, false);
    assert(!pressed_positions[0] && keypress_release_deadline_ms[0] == 0 &&
           scheduled_renders == scheduled_before_release);
    state.on = true;
    expect("release while off cannot leave a stuck key after on", test_now_ms, false, false);
    test_now_ms = UINT32_MAX - 199U;
    led_matrix_handle_position_event(0, true);
    led_matrix_handle_position_event(0, false);
    assert(keypress_release_deadline_ms[0] == 1U);
    expect("zero-wrapping release deadline remains active", test_now_ms, true, true);
    expect("zero-wrapping release deadline expires", 1U, false, false);
    reset();
    profile.renderer = LED_MATRIX_RENDERER_RIPPLE;
    base_colors[0] = LM_WHITE;
    idle_suspended = true;
    test_now_ms = 2000;
    led_matrix_handle_position_event(2, true);
    assert(!idle_suspended && ripple_state.active && ripple_state.origin_cell == 2 &&
           ripple_state.start_time_ms == test_now_ms && scheduled_renders == 1);
    expect("idle first real event starts ripple", test_now_ms, true, true);
    expect("event ripple ends", 10000, false, false);
}
int main(void) {
    static_and_animation();
    status_selection();
    overwrite_priority();
    keypress_and_ripple();
    runtime_gates();
    rain_resume_is_bounded();
    position_event_lifecycle();
    printf("PASS demand: %u production composition checks (current colors, masking, dark animation "
           "phases, keypress/ripple expiry and wake)\n",
           checks);
    return 0;
}
