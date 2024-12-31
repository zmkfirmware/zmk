/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <math.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/led_strip.h>
#include <drivers/ext_power.h>

#include <zmk/rgb_underglow.h>

#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_CHOSEN(zmk_underglow)

#error "A zmk,underglow chosen node must be declared"

#endif

#if DT_HAS_CHOSEN(zmk_underglow_transform)
#define ZMK_UNDERGLOW_TRANSFORM_NODE DT_CHOSEN(zmk_underglow_transform)
#define ZMK_UNDERGLOW_ROWS DT_PROP(ZMK_UNDERGLOW_TRANSFORM_NODE, rows)
#define ZMK_UNDERGLOW_COLS DT_PROP(ZMK_UNDERGLOW_TRANSFORM_NODE, columns)
#define ZMK_UNDERGLOW_TRANSFORM_LENGTH DT_PROP_LEN(ZMK_UNDERGLOW_TRANSFORM_NODE, map)
static int underglow_transform[] = DT_PROP(ZMK_UNDERGLOW_TRANSFORM_NODE, map);
#endif

#define STRIP_CHOSEN DT_CHOSEN(zmk_underglow)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_CHOSEN, chain_length)

#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100
#define TICKS_PER_SECOND 30
static struct led_rgb BLACK = (struct led_rgb){r : 0, g : 0, b : 0};

BUILD_ASSERT(CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN <= CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX,
             "ERROR: RGB underglow maximum brightness is less than minimum brightness");

struct rgb_underglow_effect {
    void (*tick_function)(void);
    void (*event_listener)(const zmk_event_t *);
};

struct rgb_underglow_state {
    struct zmk_led_hsb color;
    uint8_t animation_speed;
    uint8_t current_effect;
    uint16_t animation_step;
    bool on;
    bool active;
};

static const struct device *led_strip;

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static struct rgb_underglow_state state;

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
static const struct device *const ext_power = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));
#endif

static void zmk_rgb_underglow_deactivate(void) {
    state.active = false;
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
    if (ext_power != NULL) {
        int rc = ext_power_disable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to disable EXT_POWER: %d", rc);
        }
    }
#endif
}

static void zmk_rgb_underglow_activate(void) {
    state.active = true;
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
    if (ext_power != NULL) {
        int rc = ext_power_enable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to enable EXT_POWER: %d", rc);
        }
    }
#endif
}
static struct zmk_led_hsb hsb_scale_min_max(struct zmk_led_hsb hsb) {
    hsb.b = CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN +
            (CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX - CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN) * hsb.b / BRT_MAX;
    return hsb;
}

static struct zmk_led_hsb hsb_scale_zero_max(struct zmk_led_hsb hsb) {
    hsb.b = hsb.b * CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX / BRT_MAX;
    return hsb;
}

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

    struct led_rgb rgb = {r : r * 255, g : g * 255, b : b * 255};

    return rgb;
}

#ifdef ZMK_UNDERGLOW_TRANSFORM_NODE
static int keymap_pos_to_led_index(int pos) {
    int index = 0;
    for (int i = 0; i < ZMK_UNDERGLOW_TRANSFORM_LENGTH; i++) {
        if (underglow_transform[i] != -1 && underglow_transform[i] <= STRIP_NUM_PIXELS * 2) {
            if (index == pos) {
                return i;
            }
            index++;
        }
    }
    return -1;
}

static int keymap_pos_to_row(int pos) { return keymap_pos_to_led_index(pos) / ZMK_UNDERGLOW_COLS; }

static int keymap_pos_to_col(int pos) { return keymap_pos_to_led_index(pos) % ZMK_UNDERGLOW_COLS; }
#endif

static void fade_all_leds(void) {
    int active_leds = 0;
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        int r = (pixels[i].r > 5) ? pixels[i].r * 0.97 : 0;
        int g = (pixels[i].g > 5) ? pixels[i].g * 0.97 : 0;
        int b = (pixels[i].b > 5) ? pixels[i].b * 0.97 : 0;
        pixels[i] = (struct led_rgb){r : r, g : g, b : b};
        if (r > 0 || g > 0 || b > 0) {
            active_leds++;
        }
    }
    if (state.active && active_leds == 0) {
        zmk_rgb_underglow_deactivate();
    }
}

static int get_index(int row, int col) {
#ifdef ZMK_UNDERGLOW_TRANSFORM_NODE
    int transform_index = row * ZMK_UNDERGLOW_COLS + col;
    if (transform_index >= ZMK_UNDERGLOW_TRANSFORM_LENGTH) {
        return -EINVAL;
    }
    int index = underglow_transform[transform_index];
#else
    int index = row + col;
#endif
    return index;
}

static void set_led(int row, int col, struct led_rgb color) {
    int index = get_index(row, col);
    if (index < 0 || index > STRIP_NUM_PIXELS * 2) {
        return;
    }
    if (color.r > 0 || color.g > 0 || color.b > 0) {
        zmk_rgb_underglow_activate();
    }
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (index >= STRIP_NUM_PIXELS) {
        pixels[index % STRIP_NUM_PIXELS] = color;
    }
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (index < STRIP_NUM_PIXELS) {
        pixels[index] = color;
    }
#endif
#else
    pixels[index % STRIP_NUM_PIXELS] = color;
#endif
}

static struct led_rgb get_led(int row, int col) {
    int index = get_index(row, col);
    if (index < 0) {
        return BLACK;
    }
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (index >= STRIP_NUM_PIXELS) {
        return pixels[index % STRIP_NUM_PIXELS];
    } else {
        return BLACK;
    }
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (index < STRIP_NUM_PIXELS) {
        return pixels[index];
    } else {
        return BLACK;
    }
#endif
#else
    return pixels[index];
#endif
}

static void zmk_rgb_underglow_effect_solid(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        pixels[i] = hsb_to_rgb(hsb_scale_min_max(state.color));
    }
}

static void zmk_rgb_underglow_effect_breathe(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        struct zmk_led_hsb hsb = state.color;
        hsb.b = abs(state.animation_step - 1200) / 12;

        pixels[i] = hsb_to_rgb(hsb_scale_zero_max(hsb));
    }

    state.animation_step += state.animation_speed * 10;

    if (state.animation_step > 2400) {
        state.animation_step = 0;
    }
}

static void zmk_rgb_underglow_effect_spectrum(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        struct zmk_led_hsb hsb = state.color;
        hsb.h = state.animation_step;

        pixels[i] = hsb_to_rgb(hsb_scale_min_max(hsb));
    }

    state.animation_step += state.animation_speed;
    state.animation_step = state.animation_step % HUE_MAX;
}

static void zmk_rgb_underglow_effect_swirl(void) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        struct zmk_led_hsb hsb = state.color;
        hsb.h = (HUE_MAX / STRIP_NUM_PIXELS * i + state.animation_step) % HUE_MAX;

        pixels[i] = hsb_to_rgb(hsb_scale_min_max(hsb));
    }

    state.animation_step += state.animation_speed * 2;
    state.animation_step = state.animation_step % HUE_MAX;
}

static bool just_dimmed(int row, int col) {
    double window_start = 0.6;
    double window_length = 0.1;
    struct led_rgb led_val = get_led(row, col);
    struct led_rgb state_col = hsb_to_rgb(hsb_scale_zero_max(state.color));
    return led_val.r >= state_col.r * window_start && led_val.g >= state_col.g * window_start &&
           led_val.b >= state_col.b * window_start &&
           led_val.r <= state_col.r * (window_start + window_length) &&
           led_val.g <= state_col.g * (window_start + window_length) &&
           led_val.b <= state_col.b * (window_start + window_length);
}

static void zmk_rgb_underglow_effect_matrix(void) {
    fade_all_leds();
    int num_pixels = STRIP_NUM_PIXELS;
    if (IS_ENABLED(CONFIG_ZMK_SPLIT)) {
        num_pixels = num_pixels * 2;
    }
#ifdef ZMK_UNDERGLOW_TRANSFORM_NODE
    for (int col = 0; col < ZMK_UNDERGLOW_COLS; col++) {
        for (int row = 0; row < ZMK_UNDERGLOW_ROWS; row++) {
            if (just_dimmed(row, col) && row + 1 < ZMK_UNDERGLOW_ROWS) {
                set_led(row + 1, col, hsb_to_rgb(hsb_scale_zero_max(state.color)));
            } else if (row == 0 && rand() % 250 == 0) {
                set_led(row, col, hsb_to_rgb(hsb_scale_zero_max(state.color)));
            }
        }
    }
#else
    int index = state.animation_step / 10;
    set_led(0, index, state.color);
#endif

    state.animation_step++;
    state.animation_step = state.animation_step % (num_pixels * 10);
}

static void glitter_position_state_changed(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *pos_ev;
    if ((pos_ev = as_zmk_position_state_changed(eh)) != NULL && pos_ev->state) {
#ifdef ZMK_UNDERGLOW_TRANSFORM_NODE
        int index = keymap_pos_to_led_index(pos_ev->position);
        if (index == -1) {
            return;
        }
#else
        int index = pos_ev->position;
#endif
        struct zmk_led_hsb color = state.color;
        color.h = rand() % 360;
        set_led(0, index, hsb_to_rgb(hsb_scale_zero_max(color)));
    }
}

#ifdef ZMK_UNDERGLOW_TRANSFORM_NODE
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void light_circle_leds(int x_center, int y_center, int radius, struct led_rgb color) {
    if (radius == 0) {
        set_led(y_center, x_center, color);
        return;
    }
    float num_pixels = 4 + (radius - 1) * 8;
    for (int i = 0; i < num_pixels; i++) {
        double angle = 2 * M_PI * (i / num_pixels);
        int x = round(radius * cos(angle)) + x_center;
        int y = round(radius * sin(angle)) + y_center;

        if (x < ZMK_UNDERGLOW_COLS && y < ZMK_UNDERGLOW_ROWS && x >= 0 && y >= 0) {
            set_led(y, x, color);
        }
    }
}
#endif

#ifdef ZMK_UNDERGLOW_TRANSFORM_NODE
#define RIPPLE_STEPS 100
#define RIPPLE_LENGTH 5
#define MAX_RIPPLE_AGE                                                                             \
    RIPPLE_STEPS *(fmax(ZMK_UNDERGLOW_COLS, ZMK_UNDERGLOW_ROWS) + (RIPPLE_LENGTH + 1) / 2)
#define MAX_RIPPLES 3
static int ripple_positions[MAX_RIPPLES];
static int ripple_ages[MAX_RIPPLES];

static int get_brightness(float stage, int radius) {
    int x = radius;
    if (x < fmax(stage - RIPPLE_LENGTH, 0) || x > stage) {
        return 0;
    }
    float val = pow((x + RIPPLE_LENGTH - stage), 2) * log10f(-x + 1 + stage);
    return fmin(val, RIPPLE_LENGTH) * 100 / RIPPLE_LENGTH;
}

#endif

static void ripple_position_state_changed(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *pos_ev;
    if ((pos_ev = as_zmk_position_state_changed(eh)) != NULL && pos_ev->state) {
#ifdef ZMK_UNDERGLOW_TRANSFORM_NODE
        if (keymap_pos_to_row(pos_ev->position) >= 0 && keymap_pos_to_col(pos_ev->position) >= 0) {
            for (int i = 0; i < MAX_RIPPLES; i++) {
                if (ripple_positions[i] == -1) {
                    ripple_positions[i] = pos_ev->position;
                    ripple_ages[i] = MAX_RIPPLE_AGE;
                    break;
                }
            }
        }
#else
        int index = pos_ev->position;
        struct zmk_led_hsb color = state.color;
        color.h = rand() % 360;
        set_led(0, index, hsb_to_rgb(hsb_scale_zero_max(color)));
#endif
    }
}

static void ripple() {
    fade_all_leds();
#ifdef ZMK_UNDERGLOW_TRANSFORM_NODE
    float seconds_per_ripple = 2;
    struct zmk_led_hsb color = state.color;
    color.h = state.animation_step % 360;
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (ripple_positions[i] != -1) {
            ripple_ages[i] -= (int)(MAX_RIPPLE_AGE / TICKS_PER_SECOND / seconds_per_ripple);

            int stage = (MAX_RIPPLE_AGE - ripple_ages[i]) / 100.0;
            int min_radius = fmax(stage - RIPPLE_LENGTH, 0);

            for (int j = 0; j < RIPPLE_LENGTH; j++) {
                color.b = get_brightness(stage, min_radius + j);
                light_circle_leds(keymap_pos_to_col(ripple_positions[i]),
                                  keymap_pos_to_row(ripple_positions[i]), min_radius + j,
                                  hsb_to_rgb(hsb_scale_zero_max(color)));
            }

            if (ripple_ages[i] < 0) {
                ripple_positions[i] = -1;
            }
        }
    }
    state.animation_step = (state.animation_step + 1) % 360;
#endif
}

static const struct rgb_underglow_effect effects[] = {
    {&zmk_rgb_underglow_effect_solid, NULL},    {&zmk_rgb_underglow_effect_breathe, NULL},
    {&zmk_rgb_underglow_effect_spectrum, NULL}, {&zmk_rgb_underglow_effect_swirl, NULL},
    {&zmk_rgb_underglow_effect_matrix, NULL},   {&fade_all_leds, &glitter_position_state_changed},
    {&ripple, &ripple_position_state_changed},
};

static void zmk_rgb_underglow_tick(struct k_work *work) {
    if (effects[state.current_effect].tick_function != NULL) {
        effects[state.current_effect].tick_function();
    }

    int err = led_strip_update_rgb(led_strip, pixels, STRIP_NUM_PIXELS);
    if (err < 0) {
        LOG_ERR("Failed to update the RGB strip (%d)", err);
    }
}

K_WORK_DEFINE(underglow_tick_work, zmk_rgb_underglow_tick);

static void zmk_rgb_underglow_tick_handler(struct k_timer *timer) {
    if (!state.on) {
        return;
    }

    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_tick_work);
}

K_TIMER_DEFINE(underglow_tick, zmk_rgb_underglow_tick_handler, NULL);

#if IS_ENABLED(CONFIG_SETTINGS)
static int rgb_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(state)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &state, sizeof(state));
        if (rc >= 0) {
            if (state.on) {
                k_timer_start(&underglow_tick, K_NO_WAIT, K_MSEC(50));
            }

            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(rgb_underglow, "rgb/underglow", NULL, rgb_settings_set, NULL, NULL);

static void zmk_rgb_underglow_save_state_work(struct k_work *_work) {
    settings_save_one("rgb/underglow/state", &state, sizeof(state));
}

static struct k_work_delayable underglow_save_work;
#endif

static int zmk_rgb_underglow_init(void) {
    led_strip = DEVICE_DT_GET(STRIP_CHOSEN);

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
    if (!device_is_ready(ext_power)) {
        LOG_ERR("External power device \"%s\" is not ready", ext_power->name);
        return -ENODEV;
    }
#endif

    state = (struct rgb_underglow_state){
        color : {
            h : CONFIG_ZMK_RGB_UNDERGLOW_HUE_START,
            s : CONFIG_ZMK_RGB_UNDERGLOW_SAT_START,
            b : CONFIG_ZMK_RGB_UNDERGLOW_BRT_START,
        },
        animation_speed : CONFIG_ZMK_RGB_UNDERGLOW_SPD_START,
        current_effect : CONFIG_ZMK_RGB_UNDERGLOW_EFF_START,
        animation_step : 0,
        on : IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_ON_START)
    };

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&underglow_save_work, zmk_rgb_underglow_save_state_work);
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
    state.on = zmk_usb_is_powered();
#endif

    if (state.on) {
        k_timer_start(&underglow_tick, K_NO_WAIT, K_MSEC(50));
    }

    return 0;
}

int zmk_rgb_underglow_save_state(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = k_work_reschedule(&underglow_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
    return MIN(ret, 0);
#else
    return 0;
#endif
}

int zmk_rgb_underglow_get_state(bool *on_off) {
    if (!led_strip)
        return -ENODEV;

    *on_off = state.on;
    return 0;
}

int zmk_rgb_underglow_on(void) {
    if (!led_strip)
        return -ENODEV;

    zmk_rgb_underglow_activate();

    state.on = true;
    state.animation_step = 0;
    k_timer_start(&underglow_tick, K_NO_WAIT, K_MSEC(50));

    return zmk_rgb_underglow_save_state();
}

static void zmk_rgb_underglow_off_handler(struct k_work *work) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        pixels[i] = (struct led_rgb){r : 0, g : 0, b : 0};
    }

    led_strip_update_rgb(led_strip, pixels, STRIP_NUM_PIXELS);
}

K_WORK_DEFINE(underglow_off_work, zmk_rgb_underglow_off_handler);

int zmk_rgb_underglow_off(void) {
    if (!led_strip)
        return -ENODEV;

    zmk_rgb_underglow_deactivate();
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_off_work);

    k_timer_stop(&underglow_tick);
    state.on = false;

    return zmk_rgb_underglow_save_state();
}

int zmk_rgb_underglow_calc_effect(int direction) {
    const int NUM_EFFECTS = sizeof(effects) / sizeof(effects[0]);
    return (state.current_effect + NUM_EFFECTS + direction) % NUM_EFFECTS;
}

int zmk_rgb_underglow_select_effect(int effect) {
    const int NUM_EFFECTS = sizeof(effects) / sizeof(effects[0]);
    if (!led_strip)
        return -ENODEV;

    if (effect < 0 || effect >= NUM_EFFECTS) {
        return -EINVAL;
    }

    state.current_effect = effect;
    state.animation_step = 0;

    return zmk_rgb_underglow_save_state();
}

int zmk_rgb_underglow_cycle_effect(int direction) {
    return zmk_rgb_underglow_select_effect(zmk_rgb_underglow_calc_effect(direction));
}

int zmk_rgb_underglow_toggle(void) {
    return state.on ? zmk_rgb_underglow_off() : zmk_rgb_underglow_on();
}

int zmk_rgb_underglow_set_hsb(struct zmk_led_hsb color) {
    if (color.h > HUE_MAX || color.s > SAT_MAX || color.b > BRT_MAX) {
        return -ENOTSUP;
    }

    state.color = color;

    return 0;
}

struct zmk_led_hsb zmk_rgb_underglow_calc_hue(int direction) {
    struct zmk_led_hsb color = state.color;

    color.h += HUE_MAX + (direction * CONFIG_ZMK_RGB_UNDERGLOW_HUE_STEP);
    color.h %= HUE_MAX;

    return color;
}

struct zmk_led_hsb zmk_rgb_underglow_calc_sat(int direction) {
    struct zmk_led_hsb color = state.color;

    int s = color.s + (direction * CONFIG_ZMK_RGB_UNDERGLOW_SAT_STEP);
    if (s < 0) {
        s = 0;
    } else if (s > SAT_MAX) {
        s = SAT_MAX;
    }
    color.s = s;

    return color;
}

struct zmk_led_hsb zmk_rgb_underglow_calc_brt(int direction) {
    struct zmk_led_hsb color = state.color;

    int b = color.b + (direction * CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP);
    color.b = CLAMP(b, 0, BRT_MAX);

    return color;
}

int zmk_rgb_underglow_change_hue(int direction) {
    if (!led_strip)
        return -ENODEV;

    state.color = zmk_rgb_underglow_calc_hue(direction);

    return zmk_rgb_underglow_save_state();
}

int zmk_rgb_underglow_change_sat(int direction) {
    if (!led_strip)
        return -ENODEV;

    state.color = zmk_rgb_underglow_calc_sat(direction);

    return zmk_rgb_underglow_save_state();
}

int zmk_rgb_underglow_change_brt(int direction) {
    if (!led_strip)
        return -ENODEV;

    state.color = zmk_rgb_underglow_calc_brt(direction);

    return zmk_rgb_underglow_save_state();
}

int zmk_rgb_underglow_change_spd(int direction) {
    if (!led_strip)
        return -ENODEV;

    if (state.animation_speed == 1 && direction < 0) {
        return 0;
    }

    state.animation_speed += direction;

    if (state.animation_speed > 5) {
        state.animation_speed = 5;
    }

    return zmk_rgb_underglow_save_state();
}

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE) ||                                          \
    IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
struct rgb_underglow_sleep_state {
    bool is_awake;
    bool rgb_state_before_sleeping;
};

static int rgb_underglow_auto_state(bool target_wake_state) {
    static struct rgb_underglow_sleep_state sleep_state = {
        is_awake : true,
        rgb_state_before_sleeping : false
    };

    // wake up event while awake, or sleep event while sleeping -> no-op
    if (target_wake_state == sleep_state.is_awake) {
        return 0;
    }
    sleep_state.is_awake = target_wake_state;

    if (sleep_state.is_awake) {
        if (sleep_state.rgb_state_before_sleeping) {
            return zmk_rgb_underglow_on();
        } else {
            return zmk_rgb_underglow_off();
        }
    } else {
        sleep_state.rgb_state_before_sleeping = state.on;
        return zmk_rgb_underglow_off();
    }
}

static int rgb_underglow_event_listener(const zmk_event_t *eh) {

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE)
    if (as_zmk_activity_state_changed(eh)) {
        return rgb_underglow_auto_state(zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
    if (as_zmk_usb_conn_state_changed(eh)) {
        return rgb_underglow_auto_state(zmk_usb_is_powered());
    }
#endif

    return -ENOTSUP;
}

ZMK_LISTENER(rgb_underglow, rgb_underglow_event_listener);
#endif // IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE) ||
       // IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE)
ZMK_SUBSCRIPTION(rgb_underglow, zmk_activity_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
ZMK_SUBSCRIPTION(rgb_underglow, zmk_usb_conn_state_changed);
#endif

static int zmk_underglow_position_event_listener(const zmk_event_t *eh) {
    if (effects[state.current_effect].event_listener != NULL) {
        effects[state.current_effect].event_listener(eh);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(rgb_underglow_pos, zmk_underglow_position_event_listener);
ZMK_SUBSCRIPTION(rgb_underglow_pos, zmk_position_state_changed);

SYS_INIT(zmk_rgb_underglow_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
