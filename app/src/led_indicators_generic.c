/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <logging/log.h>

#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/led_indicators.h>
#include <zmk/events/led_indicator_changed.h>
#include <zmk/events/endpoint_selection_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define NUM_USB_PROFILES (COND_CODE_1(IS_ENABLED(CONFIG_ZMK_USB), (1), (0)))
#define NUM_BLE_PROFILES (COND_CODE_1(IS_ENABLED(CONFIG_ZMK_BLE), (ZMK_BLE_PROFILE_COUNT), (0)))
#define NUM_PROFILES (NUM_USB_PROFILES + NUM_BLE_PROFILES)

static zmk_leds_flags_t led_flags[NUM_PROFILES];

static size_t profile_index(enum zmk_endpoint endpoint, uint8_t profile) {
    switch (endpoint) {
    case ZMK_ENDPOINT_USB:
        return 0;
    case ZMK_ENDPOINT_BLE:
        return NUM_USB_PROFILES + profile;
    }

    CODE_UNREACHABLE;
}

zmk_leds_flags_t zmk_leds_get_current_flags() {
    enum zmk_endpoint endpoint = zmk_endpoints_selected();
    uint8_t profile = 0;

#if IS_ENABLED(CONFIG_ZMK_BLE)
    if (endpoint == ZMK_ENDPOINT_BLE) {
        profile = zmk_ble_active_profile_index();
    }
#endif

    return zmk_leds_get_flags(endpoint, profile);
}

zmk_leds_flags_t zmk_leds_get_flags(enum zmk_endpoint endpoint, uint8_t profile) {
    size_t index = profile_index(endpoint, profile);
    return led_flags[index];
}

static void raise_led_changed_event(struct k_work *_work) {
    ZMK_EVENT_RAISE(
        new_zmk_led_changed((struct zmk_led_changed){.leds = zmk_leds_get_current_flags()}));
}

static K_WORK_DEFINE(led_changed_work, raise_led_changed_event);

void zmk_leds_update_flags(zmk_leds_flags_t leds, enum zmk_endpoint endpoint, uint8_t profile) {
    size_t index = profile_index(endpoint, profile);
    led_flags[index] = leds;

    k_work_submit(&led_changed_work);
}

void zmk_leds_process_report(struct zmk_hid_led_report_body *report, enum zmk_endpoint endpoint,
                             uint8_t profile) {
    zmk_leds_flags_t leds = report->leds;
    zmk_leds_update_flags(leds, endpoint, profile);

    LOG_DBG("Update LED indicators: endpoint=%d, profile=%d, flags=%x", endpoint, profile, leds);
}

static int endpoint_listener(const zmk_event_t *eh) {
    raise_led_changed_event(NULL);
    return 0;
}

static ZMK_LISTENER(endpoint_listener, endpoint_listener);
static ZMK_SUBSCRIPTION(endpoint_listener, zmk_endpoint_selection_changed);
