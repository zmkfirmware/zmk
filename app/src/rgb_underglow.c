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
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/hid_indicators.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/split/bluetooth/central.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_CHOSEN(zmk_underglow)

#error "A zmk,underglow chosen node must be declared"

#endif

#define STRIP_CHOSEN DT_CHOSEN(zmk_underglow)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_CHOSEN, chain_length)

#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100

BUILD_ASSERT(CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN <= CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX,
             "ERROR: RGB underglow maximum brightness is less than minimum brightness");

enum rgb_underglow_effect {
    UNDERGLOW_EFFECT_SOLID,
    UNDERGLOW_EFFECT_BREATHE,
    UNDERGLOW_EFFECT_SPECTRUM,
    UNDERGLOW_EFFECT_SWIRL,
    UNDERGLOW_EFFECT_NUMBER // Used to track number of underglow effects
};

struct rgb_underglow_state {
    struct zmk_led_hsb color;
    uint8_t animation_speed;
    uint8_t current_effect;
    uint16_t animation_step;
    bool on;
    bool is_status_indicators_active;
};

static const struct device *led_strip;

/**
 * Updated when `zmk_status_update_pixels` is called
 *
 * This should contain rgb(0, 0, 0) for every pixel except for the
 * ones defined in underglow_indicators
 */
static struct led_rgb status_pixels[STRIP_NUM_PIXELS];
static struct led_rgb pixels[STRIP_NUM_PIXELS];

static struct rgb_underglow_state state;

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
static const struct device *const ext_power = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));
#endif

void zmk_rgb_set_ext_power(void);

// region Underglow Status Indicators

/*
 * This function updates the LED strip based on the current underglow effects
 * and any active status indicators. We blend the colors of any existing underglow (`pixels`)
 * with the colors of the status (`status_pixels`) until they are applied completely.
 */
static struct led_rgb *zmk_led_blend_status_pixels(int blend) {
    static struct led_rgb led_buffer[STRIP_NUM_PIXELS];
    int bat0 = zmk_battery_state_of_charge();

    // fast path: no status indicators, battery level OK
    if (blend == 0 && bat0 >= 20) {
        return pixels;
    }

    if (blend == 0) {
        for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
            led_buffer[i] = pixels[i];
        }
    } else if (blend >= 256) {
        // Blending steps maxed. Use status_pixels
        for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
            led_buffer[i] = status_pixels[i];
        }
    } else if (blend < 256) {
        // Blend status_pixels into pixels
        uint16_t blend_l = blend;
        uint16_t blend_r = 256 - blend;
        for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
            led_buffer[i].r =
                ((status_pixels[i].r * blend_l) >> 8) + ((pixels[i].r * blend_r) >> 8);
            led_buffer[i].g =
                ((status_pixels[i].g * blend_l) >> 8) + ((pixels[i].g * blend_r) >> 8);
            led_buffer[i].b =
                ((status_pixels[i].b * blend_l) >> 8) + ((pixels[i].b * blend_r) >> 8);
        }
    }

    return led_buffer;
}
#if defined(DT_N_S_underglow_indicators_EXISTS)

#define UNDERGLOW_INDICATORS DT_PATH(underglow_indicators)
#define UNDERGLOW_INDICATORS_PERIPHERALS DT_PATH(underglow_indicators, peripherals)

#define HAS_INDICATORS_PERIPHERALS DT_NODE_EXISTS(UNDERGLOW_INDICATORS_PERIPHERALS)
#define HAS_INDICATORS_NUM_LOCK DT_NODE_HAS_PROP(UNDERGLOW_INDICATORS, num_lock)
#define HAS_INDICATORS_CAPS_LOCK DT_NODE_HAS_PROP(UNDERGLOW_INDICATORS, caps_lock)
#define HAS_INDICATORS_SCROLL_LOCK DT_NODE_HAS_PROP(UNDERGLOW_INDICATORS, scroll_lock)
#define HAS_INDICATORS_LAYER_STATE DT_NODE_HAS_PROP(UNDERGLOW_INDICATORS, layer_state)
#define HAS_INDICATORS_BLE_PROFILES DT_NODE_HAS_PROP(UNDERGLOW_INDICATORS, ble_profiles)
#define HAS_INDICATORS_USB_STATE DT_NODE_HAS_PROP(UNDERGLOW_INDICATORS, usb_state)
#define HAS_INDICATORS_OUTPUT_FALLBACK DT_NODE_HAS_PROP(UNDERGLOW_INDICATORS, output_fallback)

#define BATTERY_LEVEL_HIGH 40
#define BATTERY_LEVEL_MEDIUM 20

#define HEXRGB(R, G, B)                                                                            \
    ((struct led_rgb){.r = (CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX * (R)) / 0xff,                        \
                      .g = (CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX * (G)) / 0xff,                        \
                      .b = (CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX * (B)) / 0xff})
static const struct led_rgb status_color_batt_low = HEXRGB(0xff, 0x00, 0x00);         // red
static const struct led_rgb status_color_batt_med = HEXRGB(0xff, 0xff, 0x00);         // yellow
static const struct led_rgb status_color_batt_high = HEXRGB(0x00, 0xff, 0x00);        // green
static const struct led_rgb status_color_batt_not_conn = HEXRGB(0xff, 0x00, 0x00);    // red
static const struct led_rgb status_color_hid = HEXRGB(0xff, 0x00, 0x00);              // red
static const struct led_rgb status_color_layer = HEXRGB(0xff, 0x00, 0xff);            // magenta
static const struct led_rgb status_color_ble_active = HEXRGB(0xff, 0xff, 0xff);       // white
static const struct led_rgb status_color_ble_connected = HEXRGB(0x00, 0xff, 0x68);    // dull-green
static const struct led_rgb status_color_ble_paired = HEXRGB(0xff, 0x00, 0x00);       // red
static const struct led_rgb status_color_ble_unused = HEXRGB(0x6b, 0x1f, 0xce);       // lilac
static const struct led_rgb status_color_usb_active = HEXRGB(0xff, 0xff, 0xff);       // white
static const struct led_rgb status_color_usb_connected = HEXRGB(0x00, 0xff, 0x68);    // dull-green
static const struct led_rgb status_color_usb_powered = HEXRGB(0xff, 0x00, 0x00);      // red
static const struct led_rgb status_color_usb_disconnected = HEXRGB(0x6b, 0x1f, 0xce); // lilac
static const struct led_rgb status_color_output_fallback = HEXRGB(0xff, 0x00, 0x00);  // red

static uint16_t status_animation_step;

/**
 * Update a buffer to reflect a given battery level using the provided indicators
 */
static void zmk_status_batt_level(struct led_rgb *led_buffer, int bat_level,
                                  const uint8_t *indicators, int indicator_count) {
    struct led_rgb bat_colour;

    if (bat_level > BATTERY_LEVEL_HIGH) {
        bat_colour = status_color_batt_high;
    } else if (bat_level > BATTERY_LEVEL_MEDIUM) {
        bat_colour = status_color_batt_med;
    } else {
        bat_colour = status_color_batt_low;
    }

    for (int i = 0; i < indicator_count; i++) {
        int min_level = (i * 100) / (indicator_count - 1);
        led_buffer[indicators[i]] =
            bat_level >= min_level ? bat_colour : (struct led_rgb){.r = 0, .g = 0, .b = 0};
    }
}

/**
 * Update a buffer with the appropriate pixels set for the battery levels on both the lhs and rhs
 */
static void zmk_status_batt_pixels(struct led_rgb *buffer) {
#if HAS_INDICATORS_PERIPHERALS
#define ZMK_STATUS_PERIPHERAL_PLUS_ONE(n) 1 +
#define ZMK_STATUS_PERIPHERAL_COUNT                                                                \
    (DT_FOREACH_CHILD(UNDERGLOW_INDICATORS_PERIPHERALS, ZMK_STATUS_PERIPHERAL_PLUS_ONE) 0)
#define ZMK_STATUS_PERIPHERAL_LED_COUNT(node_id) DT_PROP_LEN(node_id, peripheral_battery),
#define ZMK_STATUS_PERIPHERAL_LED_LIST(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),
#define ZMK_STATUS_PERIPHERAL_LEDS(node_id)                                                        \
    DT_FOREACH_PROP_ELEM(node_id, peripheral_battery, ZMK_STATUS_PERIPHERAL_LED_LIST)

    // Array with all the led-addresses for each peripheral
    static const int peripheral_led_count[] = {
        DT_FOREACH_CHILD(UNDERGLOW_INDICATORS_PERIPHERALS, ZMK_STATUS_PERIPHERAL_LED_COUNT)};

    static const uint8_t peripheral_led_addresses[] = {
        DT_FOREACH_CHILD(UNDERGLOW_INDICATORS_PERIPHERALS, ZMK_STATUS_PERIPHERAL_LEDS)};

    for (int i = 0; i < ZMK_STATUS_PERIPHERAL_COUNT; i++) {
        int offset = 0;
        for (int j = 0; j < i; j++) {
            offset += peripheral_led_count[j];
        }

        const uint8_t *addresses = &peripheral_led_addresses[offset];
        int address_count = peripheral_led_count[i];

        if (i == 0) { // Central peripheral
            zmk_status_batt_level(buffer, zmk_battery_state_of_charge(), addresses, address_count);

        } else {
            // Non-central peripherals require this config option to report battery level
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
            uint8_t peripheral_level = 0;
            int rc = zmk_split_get_peripheral_battery_level(i - 1, &peripheral_level);

            if (rc == 0) {
                zmk_status_batt_level(buffer, peripheral_level, addresses, address_count);
            } else if (rc == -ENOTCONN) {
                // Set all pixels to red
                for (int j = 0; j < address_count; j++) {
                    buffer[addresses[j]] = status_color_batt_not_conn;
                }
            } else if (rc == -EINVAL) {
                LOG_ERR("Invalid peripheral index requested for battery level read: 0");
            }
#endif
        }
    }
#endif
}

static void zmk_status_hid_pixels(struct led_rgb *buffer) {
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    zmk_hid_indicators_t led_flags = zmk_hid_indicators_get_current_profile();

#if HAS_INDICATORS_NUM_LOCK
    if (led_flags & BIT(0))
        buffer[DT_PROP(UNDERGLOW_INDICATORS, num_lock)] = status_color_hid;
#endif

#if HAS_INDICATORS_CAPS_LOCK
    if (led_flags & BIT(1))
        buffer[DT_PROP(UNDERGLOW_INDICATORS, caps_lock)] = status_color_hid;
#endif

#if HAS_INDICATORS_SCROLL_LOCK
    if (led_flags & BIT(2))
        buffer[DT_PROP(UNDERGLOW_INDICATORS, scroll_lock)] = status_color_hid;
#endif

#endif
}

static void zmk_status_layer_pixels(struct led_rgb *buffer) {
#if HAS_INDICATORS_LAYER_STATE
    static const uint8_t layer_state_indicators[] = DT_PROP(UNDERGLOW_INDICATORS, ble_profiles);
    static const int layer_state_indicator_count = DT_PROP_LEN(UNDERGLOW_INDICATORS, ble_profiles);

    for (uint8_t i = 0; i < layer_state_indicator_count; i++) {
        if (zmk_keymap_layer_active(i))
            buffer[layer_state_indicators[i]] = status_color_layer;
    }
#endif
}

static void zmk_status_ble_profile_pixels(struct led_rgb *buffer) {
#if HAS_INDICATORS_BLE_PROFILES
    static const uint8_t ble_profile_indicators[] = DT_PROP(UNDERGLOW_INDICATORS, ble_profiles);
    static const int ble_profile_indicator_count = DT_PROP_LEN(UNDERGLOW_INDICATORS, ble_profiles);

    struct zmk_endpoint_instance active_endpoint = zmk_endpoints_selected();
    int active_ble_profile_index = zmk_ble_active_profile_index();
    for (uint8_t i = 0; i < MIN(ZMK_BLE_PROFILE_COUNT, ble_profile_indicator_count); i++) {
        int8_t status = zmk_ble_profile_status(i);
        int ble_pixel_addr = ble_profile_indicators[i];
        if (status == 2 && active_endpoint.transport == ZMK_TRANSPORT_BLE &&
            active_ble_profile_index == i) { // connected AND active
            buffer[ble_pixel_addr] = status_color_ble_active;
        } else if (status == 2) { // connected
            buffer[ble_pixel_addr] = status_color_ble_connected;
        } else if (status == 1) { // paired
            buffer[ble_pixel_addr] = status_color_ble_paired;
        } else if (status == 0) { // unused
            buffer[ble_pixel_addr] = status_color_ble_unused;
        }
    }
#endif
}

static void zmk_status_usb_state_pixel(struct led_rgb *buffer) {
#if HAS_INDICATORS_USB_STATE
    static int const pixel_address = DT_PROP(UNDERGLOW_INDICATORS, usb_state);

    struct zmk_endpoint_instance active_endpoint = zmk_endpoints_selected();
    enum zmk_usb_conn_state usb_state = zmk_usb_get_conn_state();

    if (usb_state == ZMK_USB_CONN_HID && active_endpoint.transport == ZMK_TRANSPORT_USB) {
        buffer[pixel_address] = status_color_usb_active;
    } else if (usb_state == ZMK_USB_CONN_HID) {
        buffer[pixel_address] = status_color_usb_connected;
    } else if (usb_state == ZMK_USB_CONN_POWERED) {
        buffer[pixel_address] = status_color_usb_powered;
    } else if (usb_state == ZMK_USB_CONN_NONE) {
        buffer[pixel_address] = status_color_usb_disconnected;
    }
#endif
}

static void zmk_status_output_fallback_pixel(struct led_rgb *buffer) {
#if HAS_INDICATORS_OUTPUT_FALLBACK
    if (!zmk_endpoints_preferred_transport_is_active())
        buffer[DT_PROP(UNDERGLOW_INDICATORS, output_fallback)] = status_color_output_fallback;
#endif
}

/**
 * Update `status_pixels` with all the status colors
 */
static void zmk_status_update_pixels(void) {
    static struct led_rgb status_pixels_buffer[STRIP_NUM_PIXELS];

    // All pixels not used for indicating status should be blank
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        status_pixels_buffer[i] = (struct led_rgb){.r = 0, .g = 0, .b = 0};
    }

    // Update the buffer with the status indicators
    zmk_status_batt_pixels(status_pixels_buffer);
    zmk_status_hid_pixels(status_pixels_buffer);
    zmk_status_layer_pixels(status_pixels_buffer);
    zmk_status_ble_profile_pixels(status_pixels_buffer);
    zmk_status_usb_state_pixel(status_pixels_buffer);
    zmk_status_output_fallback_pixel(status_pixels_buffer);

    // Commit the buffer to status_pixels
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        status_pixels[i] = status_pixels_buffer[i];
    }
}

/**
 * Generates the blend step (out of 256) based off the current animation step
 */
static int zmk_status_blend_step(void) {
    int16_t blend = 256;
    if (status_animation_step < (500 / 25)) {
        blend = ((status_animation_step * 256) / (500 / 25));
    } else if (status_animation_step > (8000 / 25)) {
        blend = 256 - (((status_animation_step - (8000 / 25)) * 256) / (2000 / 25));
    }
    if (blend < 0)
        blend = 0;
    if (blend > 256)
        blend = 256;

    return blend;
}

static void zmk_status_write_pixels_work(struct k_work *work) {
    zmk_status_update_pixels();

    struct led_rgb *foo = zmk_led_blend_status_pixels(zmk_status_blend_step());
    int err = led_strip_update_rgb(led_strip, foo, STRIP_NUM_PIXELS);
    if (err < 0) {
        LOG_ERR("Failed to update the RGB strip (%d)", err);
    }

    if (!state.is_status_indicators_active) {
        zmk_rgb_set_ext_power();
    }
}
K_WORK_DEFINE(underglow_write_work, zmk_status_write_pixels_work);

static void zmk_rgb_underglow_status_update(struct k_timer *timer);
K_TIMER_DEFINE(underglow_status_update_timer, zmk_rgb_underglow_status_update, NULL);
static void zmk_rgb_underglow_status_update(struct k_timer *timer) {
    if (!state.is_status_indicators_active)
        return;
    status_animation_step++;
    if (status_animation_step > (10000 / 25)) {
        state.is_status_indicators_active = false;
        k_timer_stop(&underglow_status_update_timer);
    }
    if (!k_work_is_pending(&underglow_write_work))
        k_work_submit(&underglow_write_work);
}

int zmk_rgb_underglow_status(void) {
    if (!state.is_status_indicators_active) {
        status_animation_step = 0;
    } else {
        if (status_animation_step > (500 / 25)) {
            status_animation_step = 500 / 25;
        }
    }
    state.is_status_indicators_active = true;

    zmk_status_update_pixels();
    struct led_rgb *foo = zmk_led_blend_status_pixels(zmk_status_blend_step());
    int err = led_strip_update_rgb(led_strip, foo, STRIP_NUM_PIXELS);
    if (err < 0) {
        LOG_ERR("Failed to update the RGB strip (%d)", err);
    }

    zmk_rgb_set_ext_power();

    k_timer_start(&underglow_status_update_timer, K_NO_WAIT, K_MSEC(25));

    return 0;
}
#else
int zmk_rgb_underglow_status(void) { return 0; }
static int zmk_status_blend_step(void) { return 0; }
static void zmk_status_update_pixels(void){};
#endif
// endregion

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

static void zmk_rgb_underglow_tick(struct k_work *work) {
    switch (state.current_effect) {
    case UNDERGLOW_EFFECT_SOLID:
        zmk_rgb_underglow_effect_solid();
        break;
    case UNDERGLOW_EFFECT_BREATHE:
        zmk_rgb_underglow_effect_breathe();
        break;
    case UNDERGLOW_EFFECT_SPECTRUM:
        zmk_rgb_underglow_effect_spectrum();
        break;
    case UNDERGLOW_EFFECT_SWIRL:
        zmk_rgb_underglow_effect_swirl();
        break;
    }

    struct led_rgb *write_pixels;
    if (state.is_status_indicators_active == true) {
        zmk_status_update_pixels();
        write_pixels = zmk_led_blend_status_pixels(zmk_status_blend_step());
    } else {
        write_pixels = pixels;
    }

    int err = led_strip_update_rgb(led_strip, write_pixels, STRIP_NUM_PIXELS);
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

void zmk_rgb_set_ext_power(void) {
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
    if (ext_power == NULL)
        return;
    int c_power = ext_power_get(ext_power);
    if (c_power < 0) {
        LOG_ERR("Unable to examine EXT_POWER: %d", c_power);
        c_power = 0;
    }
    int desired_state = state.on || state.is_status_indicators_active;
    if (desired_state && !c_power) {
        int rc = ext_power_enable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to enable EXT_POWER: %d", rc);
        }
    } else if (!desired_state && c_power) {
        int rc = ext_power_disable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to disable EXT_POWER: %d", rc);
        }
    }
#endif
}

int zmk_rgb_underglow_on(void) {
    if (!led_strip)
        return -ENODEV;

    state.on = true;
    zmk_rgb_set_ext_power();

    state.animation_step = 0;
    k_timer_start(&underglow_tick, K_NO_WAIT, K_MSEC(25));

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

    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_off_work);

    k_timer_stop(&underglow_tick);
    state.on = false;
    zmk_rgb_set_ext_power();

    return zmk_rgb_underglow_save_state();
}

int zmk_rgb_underglow_calc_effect(int direction) {
    return (state.current_effect + UNDERGLOW_EFFECT_NUMBER + direction) % UNDERGLOW_EFFECT_NUMBER;
}

int zmk_rgb_underglow_select_effect(int effect) {
    if (!led_strip)
        return -ENODEV;

    if (effect < 0 || effect >= UNDERGLOW_EFFECT_NUMBER) {
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

SYS_INIT(zmk_rgb_underglow_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
