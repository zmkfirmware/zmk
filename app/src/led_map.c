/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_map

#include <string.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/battery.h>
#include <zmk/keymap.h>
#include <zmk/rgb_underglow.h>
#include <zmk/workqueue.h>

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#include <zmk/hid_indicators.h>
#include <zmk/events/hid_indicators_changed.h>
#include <dt-bindings/zmk/hid_usage.h>
#define CAPS_LOCK_BIT HID_USAGE_LED_CAPS_LOCK
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_ext_power_generic)
#include <drivers/ext_power.h>
static const struct device *const ext_power_dev = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#endif

#include <zmk/events/layer_state_changed.h>
#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/endpoints.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_COMPAT_STATUS_OKAY(zmk_led_map)
#error "No zmk,led-map node found in devicetree"
#endif

#define LED_MAP_NODE DT_INST(0, zmk_led_map)
#define LED_STRIP_NODE DT_PHANDLE(LED_MAP_NODE, led_strip)
#define TOTAL_LEDS DT_PROP(LED_STRIP_NODE, chain_length)

/*
 * LED array maps are defined in the board-specific header since
 * Zephyr 4.1 DTS bindings don't generate macros for type: array
 * properties in module bindings.
 */
#if defined(CONFIG_BOARD_DERIVATIVE_REV_A)
#include <derivative_rev_a_led_map.h>
#else
#error "No board-specific LED map header configured"
#endif

/* Indicator LED indices from DTS (int properties work fine) */
#if DT_NODE_HAS_PROP(LED_MAP_NODE, caps_lock_led_index)
#define CAPS_LED_INDEX DT_PROP(LED_MAP_NODE, caps_lock_led_index)
#else
#define CAPS_LED_INDEX -1
#endif

#if DT_NODE_HAS_PROP(LED_MAP_NODE, bt_status_led_index)
#define BT_LED_INDEX DT_PROP(LED_MAP_NODE, bt_status_led_index)
#else
#define BT_LED_INDEX -1
#endif

#if DT_NODE_HAS_PROP(LED_MAP_NODE, layer_status_led_index)
#define LAYER_LED_INDEX DT_PROP(LED_MAP_NODE, layer_status_led_index)
#else
#define LAYER_LED_INDEX -1
#endif

/* Per-key effect types */
enum per_key_effect {
    PER_KEY_EFFECT_SOLID,
    PER_KEY_EFFECT_BREATHE,
    PER_KEY_EFFECT_SPECTRUM,
    PER_KEY_EFFECT_SWIRL,
    PER_KEY_EFFECT_REACTIVE_FADE_FULL,
    PER_KEY_EFFECT_REACTIVE_FADE_COOL,
    PER_KEY_EFFECT_REACTIVE_FADE_WARM,
    PER_KEY_EFFECT_REACTIVE_FADE_SOLID,
    PER_KEY_EFFECT_ROW_WAVE_COOL,
    PER_KEY_EFFECT_ROW_WAVE_COOL_REV,
    PER_KEY_EFFECT_ROW_WAVE_WARM,
    PER_KEY_EFFECT_ROW_WAVE_WARM_REV,
    PER_KEY_EFFECT_ROW_WAVE_FULL,
    PER_KEY_EFFECT_ROW_WAVE_FULL_REV,
    PER_KEY_EFFECT_COL_WAVE_COOL,
    PER_KEY_EFFECT_COL_WAVE_COOL_REV,
    PER_KEY_EFFECT_COL_WAVE_WARM,
    PER_KEY_EFFECT_COL_WAVE_WARM_REV,
    PER_KEY_EFFECT_COL_WAVE_FULL,
    PER_KEY_EFFECT_COL_WAVE_FULL_REV,
    PER_KEY_EFFECT_STARRY,
    PER_KEY_EFFECT_NUMBER
};

#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100

/* Underglow effect indices (must match rgb_underglow.c enum) */
#define UG_EFFECT_SOLID 0
#define UG_EFFECT_BREATHE 1
#define UG_EFFECT_SPECTRUM 2
#define UG_EFFECT_SWIRL 3

/* Unified pixel buffer -- sole owner */
static const struct device *led_strip_dev;
static struct led_rgb pixels[TOTAL_LEDS];

/* Per-key reactive state */
struct key_reactive_state {
    uint8_t brightness;
    uint16_t hue;
    bool pressed;
    uint8_t hold_ticks;    /* ticks to hold at full brightness before fading */
    uint8_t fade_in_ticks; /* ticks remaining for fade-in ramp */
};

static struct key_reactive_state key_states[PER_KEY_COUNT];

/* Per-key persistent state (independent from underglow) */
struct led_map_state {
    struct zmk_led_hsb color;
    uint8_t current_effect;
    uint8_t animation_speed;
    bool per_key_on;
    bool indicators_on;
};

static struct led_map_state lm_state;
static uint16_t pk_animation_step;
static bool led_map_idle;
static int64_t starry_next_trigger;

/* Indicator cached state */
static struct {
    bool caps_lock;
    uint8_t bt_profile_index;
    bool bt_connected;
    int64_t bt_changed_at;    /* timestamp of last BT profile change */
    int64_t bat_requested_at; /* timestamp of last battery display request */
    int64_t usb_connected_at; /* timestamp of USB connection */
    int64_t adjust_at;        /* timestamp of last per-key adjustment */
    uint8_t adjust_level;     /* normalized 0-100 for adjustment indicator */
    uint8_t active_layer;
} indicator_cache;

/* --- HSB to RGB conversion --- */

static struct led_rgb hsb_to_rgb(struct zmk_led_hsb hsb) {
    float r = 0, g = 0, b = 0;

    uint8_t i = hsb.h / 60;
    float v = hsb.b / ((float)BRT_MAX);
    float s = hsb.s / ((float)SAT_MAX);
    float f = hsb.h / ((float)HUE_MAX) * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    }

    struct led_rgb rgb = {.r = r * 255, .g = g * 255, .b = b * 255};
    return rgb;
}

static inline uint8_t scale_brt(uint8_t brt, uint8_t max_pct) {
    return (uint8_t)((uint16_t)brt * max_pct / BRT_MAX);
}

/* --- Underglow rendering --- */

static void render_underglow_leds(void) {
#if UNDERGLOW_COUNT == 0
    return;
#else
    struct zmk_rgb_underglow_render_state ug;
    if (zmk_rgb_underglow_get_render_state(&ug) != 0 || !ug.on) {
        return;
    }

    for (int i = 0; i < UNDERGLOW_COUNT; i++) {
        uint8_t led_idx = underglow_map[i];
        if (led_idx >= TOTAL_LEDS) {
            continue;
        }

        struct zmk_led_hsb hsb = ug.color;

        switch (ug.current_effect) {
        case UG_EFFECT_SOLID:
            hsb.b = CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN +
                    (CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX - CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN) * hsb.b /
                        BRT_MAX;
            break;
        case UG_EFFECT_BREATHE:
            hsb.b = abs((int)ug.animation_step - 1200) / 12;
            hsb.b = scale_brt(hsb.b, CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX);
            break;
        case UG_EFFECT_SPECTRUM:
            hsb.h = ug.animation_step;
            hsb.b = CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN +
                    (CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX - CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN) * hsb.b /
                        BRT_MAX;
            break;
        case UG_EFFECT_SWIRL:
            hsb.h = (HUE_MAX / UNDERGLOW_COUNT * i + ug.animation_step) % HUE_MAX;
            hsb.b = CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN +
                    (CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX - CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN) * hsb.b /
                        BRT_MAX;
            break;
        }

        pixels[led_idx] = hsb_to_rgb(hsb);
    }

    zmk_rgb_underglow_advance_animation();
#endif
}

/* --- Per-key rendering --- */

static void per_key_effect_solid(void) {
    struct zmk_led_hsb hsb = lm_state.color;
    hsb.b = scale_brt(hsb.b, CONFIG_ZMK_LED_MAP_PER_KEY_BRT_MAX);
    struct led_rgb color = hsb_to_rgb(hsb);

    for (int i = 0; i < PER_KEY_COUNT; i++) {
        uint8_t led_idx = per_key_map[i];
        if (led_idx < TOTAL_LEDS) {
            pixels[led_idx] = color;
        }
    }
}

static void per_key_effect_breathe(void) {
    struct zmk_led_hsb hsb = lm_state.color;
    hsb.b = abs((int)pk_animation_step - 1200) / 12;
    hsb.b = scale_brt(hsb.b, CONFIG_ZMK_LED_MAP_PER_KEY_BRT_MAX);
    struct led_rgb color = hsb_to_rgb(hsb);

    for (int i = 0; i < PER_KEY_COUNT; i++) {
        uint8_t led_idx = per_key_map[i];
        if (led_idx < TOTAL_LEDS) {
            pixels[led_idx] = color;
        }
    }

    pk_animation_step += lm_state.animation_speed * 10;
    if (pk_animation_step > 2400) {
        pk_animation_step = 0;
    }
}

static void per_key_effect_spectrum(void) {
    struct zmk_led_hsb hsb = lm_state.color;
    hsb.h = pk_animation_step;
    hsb.b = scale_brt(hsb.b, CONFIG_ZMK_LED_MAP_PER_KEY_BRT_MAX);
    struct led_rgb color = hsb_to_rgb(hsb);

    for (int i = 0; i < PER_KEY_COUNT; i++) {
        uint8_t led_idx = per_key_map[i];
        if (led_idx < TOTAL_LEDS) {
            pixels[led_idx] = color;
        }
    }

    pk_animation_step += lm_state.animation_speed;
    pk_animation_step = pk_animation_step % HUE_MAX;
}

static void per_key_effect_swirl(void) {
    for (int i = 0; i < PER_KEY_COUNT; i++) {
        struct zmk_led_hsb hsb = lm_state.color;
        hsb.h = (HUE_MAX / PER_KEY_COUNT * i + pk_animation_step) % HUE_MAX;
        hsb.b = scale_brt(hsb.b, CONFIG_ZMK_LED_MAP_PER_KEY_BRT_MAX);

        uint8_t led_idx = per_key_map[i];
        if (led_idx < TOTAL_LEDS) {
            pixels[led_idx] = hsb_to_rgb(hsb);
        }
    }

    pk_animation_step += lm_state.animation_speed * 2;
    pk_animation_step = pk_animation_step % HUE_MAX;
}

/* Reactive fade ticks based on speed 1-5: 800ms, 600ms, 400ms, 200ms, 0ms */
static inline uint8_t reactive_fade_ticks(void) {
    static const uint8_t ticks[] = {16, 12, 8, 4, 1};
    return ticks[lm_state.animation_speed - 1];
}

static void reactive_fade_render(uint16_t hue, int i) {
    uint8_t fade_total = reactive_fade_ticks();
    uint8_t brt = BRT_MAX * key_states[i].brightness / fade_total;

    struct zmk_led_hsb hsb = {.h = hue, .s = lm_state.color.s, .b = BRT_MAX};
    struct led_rgb rgb = hsb_to_rgb(hsb);
    uint32_t scale = (uint32_t)brt * lm_state.color.b * CONFIG_ZMK_LED_MAP_PER_KEY_BRT_MAX;
    uint32_t divisor = (uint32_t)BRT_MAX * 100 * 100;
    rgb.r = (uint8_t)((uint32_t)rgb.r * scale / divisor);
    rgb.g = (uint8_t)((uint32_t)rgb.g * scale / divisor);
    rgb.b = (uint8_t)((uint32_t)rgb.b * scale / divisor);
    uint8_t led_idx = per_key_map[i];
    if (led_idx < TOTAL_LEDS) {
        pixels[led_idx] = rgb;
    }
}

static void per_key_effect_reactive_fade_full(void) {
    for (int i = 0; i < PER_KEY_COUNT; i++) {
        if (key_states[i].brightness > 0) {
            reactive_fade_render(key_states[i].hue, i);
        }
    }

    for (int i = 0; i < PER_KEY_COUNT; i++) {
        if (!key_states[i].pressed && key_states[i].brightness > 0) {
            key_states[i].brightness--;
        }
    }
}

static void per_key_effect_reactive_fade_solid(void) {
    for (int i = 0; i < PER_KEY_COUNT; i++) {
        if (key_states[i].brightness > 0) {
            reactive_fade_render(lm_state.color.h, i);
        }
    }

    for (int i = 0; i < PER_KEY_COUNT; i++) {
        if (!key_states[i].pressed && key_states[i].brightness > 0) {
            key_states[i].brightness--;
        }
    }
}

#define STARRY_FADE_IN_TICKS 10  /* ~500ms linear fade in */
#define STARRY_FADE_OUT_TICKS 10 /* ~500ms linear fade out */

static void per_key_effect_starry(void) {
    /* Randomly trigger a key every 200-900ms with random color */
    int64_t now = k_uptime_get();
    if (now >= starry_next_trigger) {
        int idx = k_cycle_get_32() % PER_KEY_COUNT;
        key_states[idx].hue = (uint16_t)(k_cycle_get_32() % HUE_MAX);
        key_states[idx].fade_in_ticks = STARRY_FADE_IN_TICKS;
        key_states[idx].hold_ticks = 20 + (k_cycle_get_32() % 41); /* 1-3s */
        key_states[idx].brightness = STARRY_FADE_OUT_TICKS;        /* preload fade-out counter */
        starry_next_trigger = now + 200 + (k_cycle_get_32() % 701);
    }

    /* Render and advance phases: fade in → hold → fade out (all linear) */
    for (int i = 0; i < PER_KEY_COUNT; i++) {
        uint8_t brt;

        if (key_states[i].fade_in_ticks > 0) {
            brt = BRT_MAX * (STARRY_FADE_IN_TICKS - key_states[i].fade_in_ticks) /
                  STARRY_FADE_IN_TICKS;
            key_states[i].fade_in_ticks--;
        } else if (key_states[i].hold_ticks > 0) {
            brt = BRT_MAX;
            key_states[i].hold_ticks--;
        } else if (key_states[i].brightness > 0) {
            brt = BRT_MAX * key_states[i].brightness / STARRY_FADE_OUT_TICKS;
            key_states[i].brightness--;
        } else {
            continue;
        }

        /* Compute HSB at full brightness, then scale RGB with combined
         * brightness in one step to preserve sub-levels at low settings */
        struct zmk_led_hsb hsb = {.h = key_states[i].hue, .s = lm_state.color.s, .b = BRT_MAX};
        struct led_rgb rgb = hsb_to_rgb(hsb);
        uint32_t scale = (uint32_t)brt * lm_state.color.b * CONFIG_ZMK_LED_MAP_PER_KEY_BRT_MAX;
        uint32_t divisor = (uint32_t)BRT_MAX * 100 * 100;
        rgb.r = (uint8_t)((uint32_t)rgb.r * scale / divisor);
        rgb.g = (uint8_t)((uint32_t)rgb.g * scale / divisor);
        rgb.b = (uint8_t)((uint32_t)rgb.b * scale / divisor);
        uint8_t led_idx = per_key_map[i];
        if (led_idx < TOTAL_LEDS) {
            pixels[led_idx] = rgb;
        }
    }
}

static void per_key_effect_row_wave(uint16_t hue_start, uint16_t hue_range, int dir) {
    int key_idx = 0;
    for (int row = 0; row < PER_KEY_ROW_COUNT; row++) {
        uint16_t hue = (hue_start + (uint16_t)row * hue_range / (PER_KEY_ROW_COUNT - 1) + HUE_MAX +
                        dir * pk_animation_step) %
                       HUE_MAX;
        struct zmk_led_hsb hsb = {
            .h = hue,
            .s = lm_state.color.s,
            .b = scale_brt(lm_state.color.b, CONFIG_ZMK_LED_MAP_PER_KEY_BRT_MAX),
        };
        struct led_rgb color = hsb_to_rgb(hsb);

        for (int col = 0; col < per_key_row_sizes[row]; col++) {
            uint8_t led_idx = per_key_map[key_idx++];
            if (led_idx < TOTAL_LEDS) {
                pixels[led_idx] = color;
            }
        }
    }

    pk_animation_step += lm_state.animation_speed;
    pk_animation_step = pk_animation_step % HUE_MAX;
}

static void per_key_effect_col_wave(uint16_t hue_start, uint16_t hue_range, int dir) {
    for (int i = 0; i < PER_KEY_COUNT; i++) {
        uint16_t hue = (hue_start + (uint16_t)per_key_col[i] * hue_range / (PER_KEY_COL_MAX - 1) +
                        HUE_MAX + dir * pk_animation_step) %
                       HUE_MAX;
        struct zmk_led_hsb hsb = {
            .h = hue,
            .s = lm_state.color.s,
            .b = scale_brt(lm_state.color.b, CONFIG_ZMK_LED_MAP_PER_KEY_BRT_MAX),
        };
        uint8_t led_idx = per_key_map[i];
        if (led_idx < TOTAL_LEDS) {
            pixels[led_idx] = hsb_to_rgb(hsb);
        }
    }

    pk_animation_step += lm_state.animation_speed;
    pk_animation_step = pk_animation_step % HUE_MAX;
}

static void render_per_key_leds(void) {
    if (!lm_state.per_key_on) {
        return;
    }

    switch (lm_state.current_effect) {
    case PER_KEY_EFFECT_SOLID:
        per_key_effect_solid();
        break;
    case PER_KEY_EFFECT_BREATHE:
        per_key_effect_breathe();
        break;
    case PER_KEY_EFFECT_SPECTRUM:
        per_key_effect_spectrum();
        break;
    case PER_KEY_EFFECT_SWIRL:
        per_key_effect_swirl();
        break;
    case PER_KEY_EFFECT_REACTIVE_FADE_FULL:
    case PER_KEY_EFFECT_REACTIVE_FADE_COOL:
    case PER_KEY_EFFECT_REACTIVE_FADE_WARM:
        per_key_effect_reactive_fade_full();
        break;
    case PER_KEY_EFFECT_REACTIVE_FADE_SOLID:
        per_key_effect_reactive_fade_solid();
        break;
    case PER_KEY_EFFECT_ROW_WAVE_COOL:
        per_key_effect_row_wave(120, 120, 1);
        break;
    case PER_KEY_EFFECT_ROW_WAVE_WARM:
        per_key_effect_row_wave(0, 60, 1);
        break;
    case PER_KEY_EFFECT_ROW_WAVE_FULL:
        per_key_effect_row_wave(0, 359, 1);
        break;
    case PER_KEY_EFFECT_ROW_WAVE_COOL_REV:
        per_key_effect_row_wave(120, 120, -1);
        break;
    case PER_KEY_EFFECT_ROW_WAVE_WARM_REV:
        per_key_effect_row_wave(0, 60, -1);
        break;
    case PER_KEY_EFFECT_ROW_WAVE_FULL_REV:
        per_key_effect_row_wave(0, 359, -1);
        break;
    case PER_KEY_EFFECT_COL_WAVE_COOL:
        per_key_effect_col_wave(120, 120, 1);
        break;
    case PER_KEY_EFFECT_COL_WAVE_WARM:
        per_key_effect_col_wave(0, 60, 1);
        break;
    case PER_KEY_EFFECT_COL_WAVE_FULL:
        per_key_effect_col_wave(0, 359, 1);
        break;
    case PER_KEY_EFFECT_COL_WAVE_COOL_REV:
        per_key_effect_col_wave(120, 120, -1);
        break;
    case PER_KEY_EFFECT_COL_WAVE_WARM_REV:
        per_key_effect_col_wave(0, 60, -1);
        break;
    case PER_KEY_EFFECT_COL_WAVE_FULL_REV:
        per_key_effect_col_wave(0, 359, -1);
        break;
    case PER_KEY_EFFECT_STARRY:
        per_key_effect_starry();
        break;
    default:
        break;
    }
}

/* --- Indicator rendering --- */

static void render_indicator_leds(void) {
    if (!lm_state.indicators_on) {
        return;
    }

/* Scale an RGB value by indicator brightness percentage */
#define INDICATOR_SCALE_RGB(rgb_val)                                                               \
    ((struct led_rgb){                                                                             \
        .r = (uint8_t)((uint16_t)(rgb_val).r * CONFIG_ZMK_LED_MAP_INDICATOR_BRT_MAX / 100),        \
        .g = (uint8_t)((uint16_t)(rgb_val).g * CONFIG_ZMK_LED_MAP_INDICATOR_BRT_MAX / 100),        \
        .b = (uint8_t)((uint16_t)(rgb_val).b * CONFIG_ZMK_LED_MAP_INDICATOR_BRT_MAX / 100),        \
    })

    /* Battery display takes exclusive control of indicator LEDs.
     * Flick 3 times then off. During off phase, indicator LEDs are black. */
    {
        int64_t bat_elapsed = k_uptime_get() - indicator_cache.bat_requested_at;
        int period = CONFIG_ZMK_LED_MAP_BAT_FLICK_PERIOD;
        int total_duration = 3 * period;

        if (indicator_cache.bat_requested_at > 0 && bat_elapsed < total_duration) {
            struct led_rgb black = {.r = 0, .g = 0, .b = 0};
            bool show = (bat_elapsed % period) < (period / 2);

            struct led_rgb led0 = black, led1 = black, led2 = black;

            if (show) {
                uint8_t soc = zmk_battery_state_of_charge();
                struct led_rgb cyan = {.r = 0, .g = 255, .b = 255};
                struct led_rgb red = {.r = 255, .g = 0, .b = 0};

                if (soc >= 80) {
                    led0 = cyan;
                    led1 = cyan;
                    led2 = cyan;
                } else if (soc >= 50) {
                    led1 = cyan;
                    led2 = cyan;
                } else if (soc >= 20) {
                    led2 = cyan;
                } else {
                    led2 = red;
                }
            }

#if CAPS_LED_INDEX >= 0
            pixels[CAPS_LED_INDEX] = INDICATOR_SCALE_RGB(led0);
#endif
#if BT_LED_INDEX >= 0
            pixels[BT_LED_INDEX] = INDICATOR_SCALE_RGB(led1);
#endif
#if LAYER_LED_INDEX >= 0
            pixels[LAYER_LED_INDEX] = INDICATOR_SCALE_RGB(led2);
#endif
            return;
        }
    }

    /* USB connection animation: sweep in then out, 2 cycles
     * Step 0: LED 0
     * Step 1: LED 0,1
     * Step 2: LED 0,1,2
     * Step 3: LED 1,2
     * Step 4: LED 2
     * (5 steps per cycle)
     */
    {
        int64_t usb_elapsed = k_uptime_get() - indicator_cache.usb_connected_at;
        int period = 500;
        int steps_per_cycle = 5;
        int total_duration = steps_per_cycle * period;

        if (indicator_cache.usb_connected_at > 0 && usb_elapsed < total_duration) {
            struct led_rgb green = {.r = 0, .g = 255, .b = 0};
            int step = (int)(usb_elapsed / period) % steps_per_cycle;

#if CAPS_LED_INDEX >= 0
            if (step <= 2) {
                pixels[CAPS_LED_INDEX] = INDICATOR_SCALE_RGB(green);
            }
#endif
#if BT_LED_INDEX >= 0
            if (step >= 1 && step <= 3) {
                pixels[BT_LED_INDEX] = INDICATOR_SCALE_RGB(green);
            }
#endif
#if LAYER_LED_INDEX >= 0
            if (step >= 2) {
                pixels[LAYER_LED_INDEX] = INDICATOR_SCALE_RGB(green);
            }
#endif
            return;
        }
    }

    /* Normal indicator rendering (caps lock / adjustment, BT, layer) */

#if CAPS_LED_INDEX >= 0
    {
        int64_t adj_elapsed = k_uptime_get() - indicator_cache.adjust_at;
        if (indicator_cache.adjust_at > 0 && adj_elapsed < 1000) {
            /* Adjustment indicator: red (0) at min → purple (280) at max */
            uint16_t hue = (uint16_t)((uint32_t)indicator_cache.adjust_level * 280 / 100);
            struct zmk_led_hsb hsb = {.h = hue, .s = SAT_MAX, .b = BRT_MAX};
            pixels[CAPS_LED_INDEX] = INDICATOR_SCALE_RGB(hsb_to_rgb(hsb));
        } else if (indicator_cache.caps_lock) {
            struct led_rgb color = {
                .r = CONFIG_ZMK_LED_MAP_CAPS_R,
                .g = CONFIG_ZMK_LED_MAP_CAPS_G,
                .b = CONFIG_ZMK_LED_MAP_CAPS_B,
            };
            pixels[CAPS_LED_INDEX] = INDICATOR_SCALE_RGB(color);
        }
    }
#endif

#if BT_LED_INDEX >= 0
    {
        /* BT indicator: only active on boot or profile switch, not on USB.
         * Connected: solid for 3s then off.
         * Not connected: flick until idle timeout. */
        if (indicator_cache.bt_changed_at > 0 &&
            zmk_endpoint_get_selected().transport != ZMK_TRANSPORT_USB) {
            static const uint16_t bt_hues[] = {240, 120, 0, 60};
            uint8_t idx = indicator_cache.bt_profile_index % 4;
            struct zmk_led_hsb hsb = {.h = bt_hues[idx], .s = SAT_MAX, .b = BRT_MAX};
            struct led_rgb color = hsb_to_rgb(hsb);

            if (indicator_cache.bt_connected) {
                int64_t elapsed = k_uptime_get() - indicator_cache.bt_changed_at;
                if (elapsed < 3000) {
                    pixels[BT_LED_INDEX] = INDICATOR_SCALE_RGB(color);
                }
            } else {
                int period = CONFIG_ZMK_LED_MAP_BAT_FLICK_PERIOD;
                bool show = (k_uptime_get() % period) < (period / 2);
                if (show) {
                    pixels[BT_LED_INDEX] = INDICATOR_SCALE_RGB(color);
                }
            }
        }
    }
#endif

#if LAYER_LED_INDEX >= 0
    {
        uint8_t layer = indicator_cache.active_layer;
        if (layer > 0 && layer <= 7) {
            static const struct led_rgb layer_colors[] = {
                {.r = CONFIG_ZMK_LED_MAP_LAYER1_R,
                 .g = CONFIG_ZMK_LED_MAP_LAYER1_G,
                 .b = CONFIG_ZMK_LED_MAP_LAYER1_B},
                {.r = CONFIG_ZMK_LED_MAP_LAYER2_R,
                 .g = CONFIG_ZMK_LED_MAP_LAYER2_G,
                 .b = CONFIG_ZMK_LED_MAP_LAYER2_B},
                {.r = CONFIG_ZMK_LED_MAP_LAYER3_R,
                 .g = CONFIG_ZMK_LED_MAP_LAYER3_G,
                 .b = CONFIG_ZMK_LED_MAP_LAYER3_B},
                {.r = CONFIG_ZMK_LED_MAP_LAYER4_R,
                 .g = CONFIG_ZMK_LED_MAP_LAYER4_G,
                 .b = CONFIG_ZMK_LED_MAP_LAYER4_B},
                {.r = CONFIG_ZMK_LED_MAP_LAYER5_R,
                 .g = CONFIG_ZMK_LED_MAP_LAYER5_G,
                 .b = CONFIG_ZMK_LED_MAP_LAYER5_B},
                {.r = CONFIG_ZMK_LED_MAP_LAYER6_R,
                 .g = CONFIG_ZMK_LED_MAP_LAYER6_G,
                 .b = CONFIG_ZMK_LED_MAP_LAYER6_B},
                {.r = CONFIG_ZMK_LED_MAP_LAYER7_R,
                 .g = CONFIG_ZMK_LED_MAP_LAYER7_G,
                 .b = CONFIG_ZMK_LED_MAP_LAYER7_B},
            };
            pixels[LAYER_LED_INDEX] = INDICATOR_SCALE_RGB(layer_colors[layer - 1]);
        }
    }
#endif
}

/* --- Tick & timer management --- */

static bool led_map_timer_running;

static void led_map_tick(struct k_work *work) {
    memset(pixels, 0, sizeof(pixels));

    render_underglow_leds();
    render_per_key_leds();
    render_indicator_leds();

    led_strip_update_rgb(led_strip_dev, pixels, TOTAL_LEDS);
}

K_WORK_DEFINE(led_map_tick_work, led_map_tick);

static void led_map_tick_handler(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &led_map_tick_work);
}

K_TIMER_DEFINE(led_map_timer, led_map_tick_handler, NULL);

static void led_map_stop_timer(void) {
    if (led_map_timer_running) {
        k_timer_stop(&led_map_timer);
        led_map_timer_running = false;
        memset(pixels, 0, sizeof(pixels));
        led_strip_update_rgb(led_strip_dev, pixels, TOTAL_LEDS);
    }
}

static void led_map_start_timer(void) {
    if (!led_map_timer_running) {
        k_timer_start(&led_map_timer, K_MSEC(2), K_MSEC(50));
        led_map_timer_running = true;
    }
}

static bool led_map_ext_power_on;

static void led_map_set_ext_power(bool on) {
#if DT_HAS_COMPAT_STATUS_OKAY(zmk_ext_power_generic)
    if (on == led_map_ext_power_on) {
        return;
    }
    led_map_ext_power_on = on;
    if (on) {
        ext_power_enable(ext_power_dev);
    } else {
        ext_power_disable(ext_power_dev);
    }
#endif
}

static void led_map_check_timer(void) {
    bool need_leds;

    /* Never stop the timer when USB powered */
    if (zmk_usb_is_powered()) {
        need_leds = true;
    } else if (led_map_idle) {
        need_leds = false;
    } else {
        struct zmk_rgb_underglow_render_state ug;
        bool ug_on = (zmk_rgb_underglow_get_render_state(&ug) == 0 && ug.on);
        need_leds = lm_state.per_key_on || lm_state.indicators_on || ug_on;
    }

    if (need_leds) {
        led_map_set_ext_power(true);
        led_map_start_timer();
    } else {
        led_map_stop_timer();
        led_map_set_ext_power(false);
    }
}

/* --- Event listeners --- */

static int led_map_event_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *pos_ev = as_zmk_position_state_changed(eh);
    if (pos_ev != NULL) {
        if (pos_ev->position < PER_KEY_COUNT) {
            key_states[pos_ev->position].pressed = pos_ev->state;
            if (pos_ev->state) {
                switch (lm_state.current_effect) {
                case PER_KEY_EFFECT_REACTIVE_FADE_COOL:
                    key_states[pos_ev->position].hue = 120 + (uint16_t)(k_cycle_get_32() % 121);
                    key_states[pos_ev->position].brightness = reactive_fade_ticks();
                    break;
                case PER_KEY_EFFECT_REACTIVE_FADE_WARM:
                    key_states[pos_ev->position].hue = (uint16_t)(k_cycle_get_32() % 61);
                    key_states[pos_ev->position].brightness = reactive_fade_ticks();
                    break;
                case PER_KEY_EFFECT_REACTIVE_FADE_FULL:
                case PER_KEY_EFFECT_REACTIVE_FADE_SOLID:
                    key_states[pos_ev->position].hue = (uint16_t)(k_cycle_get_32() % HUE_MAX);
                    key_states[pos_ev->position].brightness = reactive_fade_ticks();
                    break;
                default:
                    break;
                }
            }
        }
        return ZMK_EV_EVENT_BUBBLE;
    }

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    const struct zmk_hid_indicators_changed *ind_ev = as_zmk_hid_indicators_changed(eh);
    if (ind_ev != NULL) {
        indicator_cache.caps_lock = (ind_ev->indicators & CAPS_LOCK_BIT) != 0;
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
    const struct zmk_ble_active_profile_changed *bt_ev = as_zmk_ble_active_profile_changed(eh);
    if (bt_ev != NULL) {
        indicator_cache.bt_profile_index = bt_ev->index;
        indicator_cache.bt_connected = zmk_ble_active_profile_is_connected();
        indicator_cache.bt_changed_at = k_uptime_get();
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

    const struct zmk_layer_state_changed *layer_ev = as_zmk_layer_state_changed(eh);
    if (layer_ev != NULL) {
        indicator_cache.active_layer = zmk_keymap_highest_layer_active();
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_usb_conn_state_changed *usb_ev = as_zmk_usb_conn_state_changed(eh);
    if (usb_ev != NULL) {
        if (usb_ev->conn_state == ZMK_USB_CONN_NONE) {
            indicator_cache.usb_connected_at = 0;
            indicator_cache.bt_changed_at = k_uptime_get();
        } else {
            /* CONN_POWERED or CONN_HID — suppress BT LED */
            indicator_cache.bt_changed_at = 0;
            if (usb_ev->conn_state == ZMK_USB_CONN_HID) {
                indicator_cache.usb_connected_at = k_uptime_get();
            }
        }
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (as_zmk_activity_state_changed(eh)) {
        led_map_idle = (zmk_activity_get_state() != ZMK_ACTIVITY_ACTIVE);
        led_map_check_timer();
        return ZMK_EV_EVENT_BUBBLE;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(led_map, led_map_event_listener);
ZMK_SUBSCRIPTION(led_map, zmk_position_state_changed);
ZMK_SUBSCRIPTION(led_map, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(led_map, zmk_activity_state_changed);

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
ZMK_SUBSCRIPTION(led_map, zmk_hid_indicators_changed);
#endif

ZMK_SUBSCRIPTION(led_map, zmk_usb_conn_state_changed);

#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(led_map, zmk_ble_active_profile_changed);
#endif

/* --- Settings persistence --- */

#if IS_ENABLED(CONFIG_SETTINGS)
static int led_map_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                void *cb_arg) {
    const char *next;
    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(lm_state)) {
            return -EINVAL;
        }
        int rc = read_cb(cb_arg, &lm_state, sizeof(lm_state));
        return MIN(rc, 0);
    }
    return -ENOENT;
}

static int led_map_settings_commit(void) {
    /* Sync ext_power tracking — ext_power's own settings handler may have
     * changed the actual state during settings load */
#if DT_HAS_COMPAT_STATUS_OKAY(zmk_ext_power_generic)
    led_map_ext_power_on = ext_power_get(ext_power_dev) > 0;
#endif
    led_map_check_timer();
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(led_map, "led_map", NULL, led_map_settings_set,
                               led_map_settings_commit, NULL);

static void led_map_save_work_handler(struct k_work *work) {
    settings_save_one("led_map/state", &lm_state, sizeof(lm_state));
}

static struct k_work_delayable led_map_save_work;
#endif

static int led_map_save_state(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = k_work_reschedule(&led_map_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
    return MIN(ret, 0);
#else
    return 0;
#endif
}

/* --- Public API --- */

int zmk_led_map_on(void) {
    lm_state.per_key_on = true;
    pk_animation_step = 0;
    led_map_check_timer();
    return led_map_save_state();
}

int zmk_led_map_off(void) {
    lm_state.per_key_on = false;
    led_map_check_timer();
    return led_map_save_state();
}

int zmk_led_map_toggle(void) { return lm_state.per_key_on ? zmk_led_map_off() : zmk_led_map_on(); }

int zmk_led_map_indicators_on(void) {
    lm_state.indicators_on = true;
    led_map_check_timer();
    return led_map_save_state();
}

int zmk_led_map_indicators_off(void) {
    lm_state.indicators_on = false;
    led_map_check_timer();
    return led_map_save_state();
}

int zmk_led_map_indicators_toggle(void) {
    return lm_state.indicators_on ? zmk_led_map_indicators_off() : zmk_led_map_indicators_on();
}

int zmk_led_map_select_effect(int effect) {
    if (effect < 0 || effect >= PER_KEY_EFFECT_NUMBER) {
        return -EINVAL;
    }
    lm_state.current_effect = effect;
    pk_animation_step = 0;
    memset(key_states, 0, sizeof(key_states));
    return led_map_save_state();
}

int zmk_led_map_cycle_effect(int direction) {
    /* Group representatives: first effect of each group */
    static const uint8_t groups[] = {
        PER_KEY_EFFECT_SOLID,
        PER_KEY_EFFECT_BREATHE,
        PER_KEY_EFFECT_SPECTRUM,
        PER_KEY_EFFECT_SWIRL,
        PER_KEY_EFFECT_REACTIVE_FADE_FULL,
        PER_KEY_EFFECT_ROW_WAVE_COOL,
        PER_KEY_EFFECT_COL_WAVE_COOL,
        PER_KEY_EFFECT_STARRY,
    };
    int num_groups = ARRAY_SIZE(groups);

    /* Find which group current effect belongs to */
    int cur_group = 0;
    for (int i = num_groups - 1; i >= 0; i--) {
        if (lm_state.current_effect >= groups[i]) {
            cur_group = i;
            break;
        }
    }

    int next_group = (cur_group + num_groups + direction) % num_groups;
    return zmk_led_map_select_effect(groups[next_group]);
}

int zmk_led_map_cycle_sub_effect(int direction) {
    /* Effect groups for sub-effect cycling */
    static const uint8_t group_basic[] = {
        PER_KEY_EFFECT_SOLID,
        PER_KEY_EFFECT_BREATHE,
        PER_KEY_EFFECT_SPECTRUM,
        PER_KEY_EFFECT_SWIRL,
    };
    static const uint8_t group_fade[] = {
        PER_KEY_EFFECT_REACTIVE_FADE_FULL,
        PER_KEY_EFFECT_REACTIVE_FADE_COOL,
        PER_KEY_EFFECT_REACTIVE_FADE_WARM,
        PER_KEY_EFFECT_REACTIVE_FADE_SOLID,
    };
    static const uint8_t group_row_wave[] = {
        PER_KEY_EFFECT_ROW_WAVE_COOL, PER_KEY_EFFECT_ROW_WAVE_COOL_REV,
        PER_KEY_EFFECT_ROW_WAVE_WARM, PER_KEY_EFFECT_ROW_WAVE_WARM_REV,
        PER_KEY_EFFECT_ROW_WAVE_FULL, PER_KEY_EFFECT_ROW_WAVE_FULL_REV,
    };

    const uint8_t *group = NULL;
    int group_len = 0;

    uint8_t cur = lm_state.current_effect;

    for (int i = 0; i < (int)ARRAY_SIZE(group_basic); i++) {
        if (group_basic[i] == cur) {
            group = group_basic;
            group_len = ARRAY_SIZE(group_basic);
            break;
        }
    }
    if (!group) {
        for (int i = 0; i < (int)ARRAY_SIZE(group_fade); i++) {
            if (group_fade[i] == cur) {
                group = group_fade;
                group_len = ARRAY_SIZE(group_fade);
                break;
            }
        }
    }
    if (!group) {
        for (int i = 0; i < (int)ARRAY_SIZE(group_row_wave); i++) {
            if (group_row_wave[i] == cur) {
                group = group_row_wave;
                group_len = ARRAY_SIZE(group_row_wave);
                break;
            }
        }
    }
    static const uint8_t group_col_wave[] = {
        PER_KEY_EFFECT_COL_WAVE_COOL, PER_KEY_EFFECT_COL_WAVE_COOL_REV,
        PER_KEY_EFFECT_COL_WAVE_WARM, PER_KEY_EFFECT_COL_WAVE_WARM_REV,
        PER_KEY_EFFECT_COL_WAVE_FULL, PER_KEY_EFFECT_COL_WAVE_FULL_REV,
    };
    if (!group) {
        for (int i = 0; i < (int)ARRAY_SIZE(group_col_wave); i++) {
            if (group_col_wave[i] == cur) {
                group = group_col_wave;
                group_len = ARRAY_SIZE(group_col_wave);
                break;
            }
        }
    }
    if (!group) {
        if (cur == PER_KEY_EFFECT_STARRY) {
            return 0; /* single-member group, no sub-effects */
        }
        return -ENOTSUP;
    }

    /* Find current index within group */
    int idx = 0;
    for (int i = 0; i < group_len; i++) {
        if (group[i] == cur) {
            idx = i;
            break;
        }
    }

    idx = (idx + group_len + direction) % group_len;
    return zmk_led_map_select_effect(group[idx]);
}

static void led_map_show_adjust(uint8_t level) {
    indicator_cache.adjust_at = k_uptime_get();
    indicator_cache.adjust_level = level;
}

int zmk_led_map_change_hue(int direction) {
    int h = (int)lm_state.color.h + HUE_MAX + (direction * CONFIG_ZMK_RGB_UNDERGLOW_HUE_STEP);
    lm_state.color.h = h % HUE_MAX;
    led_map_show_adjust((uint8_t)(lm_state.color.h * 100 / HUE_MAX));
    return led_map_save_state();
}

int zmk_led_map_change_sat(int direction) {
    int s = (int)lm_state.color.s + (direction * CONFIG_ZMK_RGB_UNDERGLOW_SAT_STEP);
    lm_state.color.s = CLAMP(s, 0, SAT_MAX);
    led_map_show_adjust(lm_state.color.s);
    return led_map_save_state();
}

int zmk_led_map_change_brt(int direction) {
    int brt_min = CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP * 3;
    int b = (int)lm_state.color.b + (direction * CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP);
    lm_state.color.b = CLAMP(b, brt_min, BRT_MAX);
    led_map_show_adjust(lm_state.color.b);
    return led_map_save_state();
}

int zmk_led_map_change_spd(int direction) {
    if (lm_state.animation_speed == 1 && direction < 0) {
        return 0;
    }
    int spd = (int)lm_state.animation_speed + direction;
    lm_state.animation_speed = CLAMP(spd, 1, 5);
    led_map_show_adjust(lm_state.animation_speed * 20);
    return led_map_save_state();
}

int zmk_led_map_show_battery(void) {
    indicator_cache.bat_requested_at = k_uptime_get();
    return 0;
}

/* --- Initialization --- */

static int led_map_init(void) {
    led_strip_dev = DEVICE_DT_GET(LED_STRIP_NODE);
    if (!device_is_ready(led_strip_dev)) {
        LOG_ERR("LED strip device not ready");
        return -ENODEV;
    }

    lm_state = (struct led_map_state){
        .color =
            {
                .h = CONFIG_ZMK_LED_MAP_HUE_START,
                .s = CONFIG_ZMK_LED_MAP_SAT_START,
                .b = CONFIG_ZMK_LED_MAP_BRT_START,
            },
        .current_effect = CONFIG_ZMK_LED_MAP_EFF_START,
        .animation_speed = CONFIG_ZMK_LED_MAP_SPD_START,
        .per_key_on = IS_ENABLED(CONFIG_ZMK_LED_MAP_ON_START),
        .indicators_on = IS_ENABLED(CONFIG_ZMK_LED_MAP_INDICATORS_ON_START),
    };

    pk_animation_step = 0;
    memset(pixels, 0, sizeof(pixels));
    memset(key_states, 0, sizeof(key_states));

    /* Initialize indicator cache */
    indicator_cache.active_layer = zmk_keymap_highest_layer_active();

#if IS_ENABLED(CONFIG_ZMK_BLE)
    indicator_cache.bt_profile_index = zmk_ble_active_profile_index();
    indicator_cache.bt_connected = zmk_ble_active_profile_is_connected();
    indicator_cache.bt_changed_at = k_uptime_get();
#endif

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    zmk_hid_indicators_t indicators = zmk_hid_indicators_get_current_profile();
    indicator_cache.caps_lock = (indicators & CAPS_LOCK_BIT) != 0;
#endif

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&led_map_save_work, led_map_save_work_handler);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_ext_power_generic)
    if (!device_is_ready(ext_power_dev)) {
        LOG_ERR("Ext power device not ready");
        return -ENODEV;
    }
#endif

    /* Enable ext_power and start timer with defaults;
     * settings commit handler will re-check after saved state loads */
    led_map_set_ext_power(true);
    led_map_start_timer();

    LOG_INF("LED map initialized: %d underglow, %d per-key, %d total LEDs", UNDERGLOW_COUNT,
            PER_KEY_COUNT, TOTAL_LEDS);

    return 0;
}

SYS_INIT(led_map_init, APPLICATION, 99);
