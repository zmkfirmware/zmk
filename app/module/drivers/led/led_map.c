/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_map

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_COMPAT_STATUS_OKAY(zmk_led_map)
#error "No zmk,led-map node found in devicetree"
#endif

#define LED_MAP_NODE DT_INST(0, zmk_led_map)

/* Get LED strip chain length from the referenced LED strip */
#define LED_STRIP_NODE DT_PHANDLE(LED_MAP_NODE, led_strip)
#define TOTAL_LEDS DT_PROP(LED_STRIP_NODE, chain_length)

/* Extract arrays from devicetree */
#define UNDERGLOW_COUNT DT_PROP_LEN(LED_MAP_NODE, underglow_map)
#define PER_KEY_COUNT DT_PROP_LEN(LED_MAP_NODE, per_key_map)

static const uint8_t underglow_map[UNDERGLOW_COUNT] = DT_PROP(LED_MAP_NODE, underglow_map);
static const uint8_t per_key_map[PER_KEY_COUNT] = DT_PROP(LED_MAP_NODE, per_key_map);

/* Per-key RGB effect types */
enum per_key_effect {
    PER_KEY_EFFECT_OFF,
    PER_KEY_EFFECT_SOLID,
    PER_KEY_EFFECT_REACTIVE,
    PER_KEY_EFFECT_REACTIVE_FADE,
    PER_KEY_EFFECT_BREATHE,
    PER_KEY_EFFECT_SPECTRUM,
    PER_KEY_EFFECT_SWIRL,
    PER_KEY_EFFECT_NUMBER
};

#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100

struct led_hsb {
    uint16_t h;
    uint8_t s;
    uint8_t b;
};

struct led_map_config {
    const struct device *led_strip;
    const uint8_t *underglow_map;
    size_t underglow_count;
    const uint8_t *per_key_map;
    size_t per_key_count;
    size_t total_leds;
};

struct led_map_data {
    struct led_rgb *pixels;
    struct led_hsb base_color;
    struct led_hsb reactive_color;
    uint8_t *key_brightness;      /* Per-key brightness for fade effect */
    bool *key_pressed;            /* Track pressed state per key */
    enum per_key_effect current_effect;
    uint16_t animation_step;
    uint8_t animation_speed;
    bool on;
    bool initialized;
};

static const struct led_map_config led_map_config = {
    .led_strip = DEVICE_DT_GET(LED_STRIP_NODE),
    .underglow_map = underglow_map,
    .underglow_count = UNDERGLOW_COUNT,
    .per_key_map = per_key_map,
    .per_key_count = PER_KEY_COUNT,
    .total_leds = TOTAL_LEDS,
};

static struct led_map_data led_map_data = {
    .base_color = {.h = 200, .s = 100, .b = 50},
    .reactive_color = {.h = 0, .s = 0, .b = 100},  /* White */
    .current_effect = PER_KEY_EFFECT_REACTIVE,
    .animation_step = 0,
    .animation_speed = 3,
    .on = true,
    .initialized = false,
};

/* HSB to RGB conversion */
static struct led_rgb hsb_to_rgb(struct led_hsb hsb) {
    float r = 0, g = 0, b = 0;

    uint8_t i = hsb.h / 60;
    float v = hsb.b / ((float)BRT_MAX);
    float s = hsb.s / ((float)SAT_MAX);
    float f = hsb.h / ((float)HUE_MAX) * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    }

    struct led_rgb rgb = {.r = r * 255, .g = g * 255, .b = b * 255};
    return rgb;
}

static int led_map_key_to_led(uint32_t key_position) {
    if (key_position >= led_map_config.per_key_count) {
        return -1;
    }
    return led_map_config.per_key_map[key_position];
}

static void led_map_set_key_pixel(uint32_t key_position, struct led_rgb color) {
    int led_index = led_map_key_to_led(key_position);
    if (led_index >= 0 && led_index < led_map_config.total_leds) {
        led_map_data.pixels[led_index] = color;
    }
}

/* Effect: All per-key LEDs off */
static void per_key_effect_off(void) {
    struct led_rgb off = {.r = 0, .g = 0, .b = 0};
    for (int i = 0; i < led_map_config.per_key_count; i++) {
        led_map_set_key_pixel(i, off);
    }
}

/* Effect: Solid color for all per-key LEDs */
static void per_key_effect_solid(void) {
    struct led_rgb color = hsb_to_rgb(led_map_data.base_color);
    for (int i = 0; i < led_map_config.per_key_count; i++) {
        led_map_set_key_pixel(i, color);
    }
}

/* Effect: Light up only pressed keys */
static void per_key_effect_reactive(void) {
    struct led_rgb on_color = hsb_to_rgb(led_map_data.reactive_color);
    struct led_rgb off_color = {.r = 0, .g = 0, .b = 0};

    for (int i = 0; i < led_map_config.per_key_count; i++) {
        if (led_map_data.key_pressed[i]) {
            led_map_set_key_pixel(i, on_color);
        } else {
            led_map_set_key_pixel(i, off_color);
        }
    }
}

/* Effect: Reactive with fade out */
static void per_key_effect_reactive_fade(void) {
    struct led_hsb hsb = led_map_data.reactive_color;

    for (int i = 0; i < led_map_config.per_key_count; i++) {
        if (led_map_data.key_pressed[i]) {
            led_map_data.key_brightness[i] = BRT_MAX;
        } else if (led_map_data.key_brightness[i] > 0) {
            /* Fade out */
            if (led_map_data.key_brightness[i] > 5) {
                led_map_data.key_brightness[i] -= 5;
            } else {
                led_map_data.key_brightness[i] = 0;
            }
        }

        hsb.b = led_map_data.key_brightness[i];
        led_map_set_key_pixel(i, hsb_to_rgb(hsb));
    }
}

/* Effect: Breathing effect for all per-key LEDs */
static void per_key_effect_breathe(void) {
    struct led_hsb hsb = led_map_data.base_color;
    hsb.b = abs((int)led_map_data.animation_step - 1200) / 12;

    struct led_rgb color = hsb_to_rgb(hsb);
    for (int i = 0; i < led_map_config.per_key_count; i++) {
        led_map_set_key_pixel(i, color);
    }

    led_map_data.animation_step += led_map_data.animation_speed * 10;
    if (led_map_data.animation_step > 2400) {
        led_map_data.animation_step = 0;
    }
}

/* Effect: Spectrum cycle for all per-key LEDs */
static void per_key_effect_spectrum(void) {
    struct led_hsb hsb = led_map_data.base_color;
    hsb.h = led_map_data.animation_step;

    struct led_rgb color = hsb_to_rgb(hsb);
    for (int i = 0; i < led_map_config.per_key_count; i++) {
        led_map_set_key_pixel(i, color);
    }

    led_map_data.animation_step += led_map_data.animation_speed;
    led_map_data.animation_step = led_map_data.animation_step % HUE_MAX;
}

/* Effect: Rainbow swirl across per-key LEDs */
static void per_key_effect_swirl(void) {
    struct led_hsb hsb = led_map_data.base_color;

    for (int i = 0; i < led_map_config.per_key_count; i++) {
        hsb.h = (HUE_MAX / led_map_config.per_key_count * i + led_map_data.animation_step) % HUE_MAX;
        led_map_set_key_pixel(i, hsb_to_rgb(hsb));
    }

    led_map_data.animation_step += led_map_data.animation_speed * 2;
    led_map_data.animation_step = led_map_data.animation_step % HUE_MAX;
}

static void led_map_tick(struct k_work *work) {
    if (!led_map_data.initialized || !led_map_data.on) {
        return;
    }

    switch (led_map_data.current_effect) {
    case PER_KEY_EFFECT_OFF:
        per_key_effect_off();
        break;
    case PER_KEY_EFFECT_SOLID:
        per_key_effect_solid();
        break;
    case PER_KEY_EFFECT_REACTIVE:
        per_key_effect_reactive();
        break;
    case PER_KEY_EFFECT_REACTIVE_FADE:
        per_key_effect_reactive_fade();
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
    default:
        break;
    }

    led_strip_update_rgb(led_map_config.led_strip, led_map_data.pixels,
                         led_map_config.total_leds);
}

K_WORK_DEFINE(led_map_tick_work, led_map_tick);

static void led_map_tick_handler(struct k_timer *timer) {
    if (!led_map_data.on) {
        return;
    }
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &led_map_tick_work);
}

K_TIMER_DEFINE(led_map_timer, led_map_tick_handler, NULL);

static int led_map_position_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL || !led_map_data.initialized) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->position < led_map_config.per_key_count) {
        led_map_data.key_pressed[ev->position] = ev->state;

        /* For reactive effects, trigger immediate update */
        if (led_map_data.current_effect == PER_KEY_EFFECT_REACTIVE ||
            led_map_data.current_effect == PER_KEY_EFFECT_REACTIVE_FADE) {
            k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &led_map_tick_work);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(led_map, led_map_position_state_changed_listener);
ZMK_SUBSCRIPTION(led_map, zmk_position_state_changed);

/* Public API */
int zmk_led_map_on(void) {
    led_map_data.on = true;
    k_timer_start(&led_map_timer, K_NO_WAIT, K_MSEC(50));
    return 0;
}

int zmk_led_map_off(void) {
    led_map_data.on = false;
    k_timer_stop(&led_map_timer);

    /* Turn off all per-key LEDs */
    per_key_effect_off();
    led_strip_update_rgb(led_map_config.led_strip, led_map_data.pixels,
                         led_map_config.total_leds);
    return 0;
}

int zmk_led_map_toggle(void) {
    return led_map_data.on ? zmk_led_map_off() : zmk_led_map_on();
}

int zmk_led_map_select_effect(enum per_key_effect effect) {
    if (effect >= PER_KEY_EFFECT_NUMBER) {
        return -EINVAL;
    }
    led_map_data.current_effect = effect;
    led_map_data.animation_step = 0;
    return 0;
}

int zmk_led_map_cycle_effect(int direction) {
    int effect = (led_map_data.current_effect + PER_KEY_EFFECT_NUMBER + direction) %
                 PER_KEY_EFFECT_NUMBER;
    return zmk_led_map_select_effect(effect);
}

static int led_map_init(void) {
    if (!device_is_ready(led_map_config.led_strip)) {
        LOG_ERR("LED strip device not ready");
        return -ENODEV;
    }

    /* Allocate pixel buffer */
    led_map_data.pixels = k_malloc(sizeof(struct led_rgb) * led_map_config.total_leds);
    if (!led_map_data.pixels) {
        LOG_ERR("Failed to allocate LED pixel buffer");
        return -ENOMEM;
    }

    /* Allocate per-key brightness buffer */
    led_map_data.key_brightness = k_malloc(sizeof(uint8_t) * led_map_config.per_key_count);
    if (!led_map_data.key_brightness) {
        LOG_ERR("Failed to allocate key brightness buffer");
        return -ENOMEM;
    }

    /* Allocate key pressed state buffer */
    led_map_data.key_pressed = k_malloc(sizeof(bool) * led_map_config.per_key_count);
    if (!led_map_data.key_pressed) {
        LOG_ERR("Failed to allocate key pressed buffer");
        return -ENOMEM;
    }

    /* Initialize buffers */
    memset(led_map_data.pixels, 0, sizeof(struct led_rgb) * led_map_config.total_leds);
    memset(led_map_data.key_brightness, 0, sizeof(uint8_t) * led_map_config.per_key_count);
    memset(led_map_data.key_pressed, 0, sizeof(bool) * led_map_config.per_key_count);

    led_map_data.initialized = true;

    /* Start the effect timer */
    if (led_map_data.on) {
        k_timer_start(&led_map_timer, K_NO_WAIT, K_MSEC(50));
    }

    LOG_INF("LED map initialized: %d underglow, %d per-key, %d total LEDs",
            led_map_config.underglow_count, led_map_config.per_key_count,
            led_map_config.total_leds);

    return 0;
}

SYS_INIT(led_map_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
