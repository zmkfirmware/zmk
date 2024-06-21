/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/hid_indicators.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/split/bluetooth/central.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static zmk_hid_indicators_t hid_indicators[ZMK_ENDPOINT_COUNT];

zmk_hid_indicators_t zmk_hid_indicators_get_current_profile(void) {
    return zmk_hid_indicators_get_profile(zmk_endpoints_selected());
}

zmk_hid_indicators_t zmk_hid_indicators_get_profile(struct zmk_endpoint_instance endpoint) {
    const int profile = zmk_endpoint_instance_to_index(endpoint);
    return hid_indicators[profile];
}

static void zmk_hid_indicators_send_state(struct k_work *work) {
    zmk_hid_indicators_t indicators = zmk_hid_indicators_get_current_profile();
    int err = zmk_split_central_send_data(DATA_TAG_HID_INDICATORS_STATE,
                                          sizeof(zmk_hid_indicators_t), (uint8_t *)&indicators);
    if (err) {
        LOG_ERR("HID indicators send failed (err %d)", err);
    }
}

K_WORK_DEFINE(hid_indicators_send_state_work, zmk_hid_indicators_send_state);

static void raise_led_changed_event(struct k_work *_work) {
    const zmk_hid_indicators_t indicators = zmk_hid_indicators_get_current_profile();

    raise_zmk_hid_indicators_changed((struct zmk_hid_indicators_changed){.indicators = indicators});

    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &hid_indicators_send_state_work);
}

static K_WORK_DEFINE(led_changed_work, raise_led_changed_event);

void zmk_hid_indicators_set_profile(zmk_hid_indicators_t indicators,
                                    struct zmk_endpoint_instance endpoint) {
    int profile = zmk_endpoint_instance_to_index(endpoint);

    // This write is not happening on the main thread. To prevent potential data races, every
    // operation involving hid_indicators must be atomic. Currently, each function either reads
    // or writes only one entry at a time, so it is safe to do these operations without a lock.
    hid_indicators[profile] = indicators;

    k_work_submit(&led_changed_work);
}

void zmk_hid_indicators_process_report(struct zmk_hid_led_report_body *report,
                                       struct zmk_endpoint_instance endpoint) {
    const zmk_hid_indicators_t indicators = (zmk_hid_indicators_t)report->leds;
    zmk_hid_indicators_set_profile(indicators, endpoint);

    LOG_DBG("Update HID indicators: endpoint=%d, indicators=%x", endpoint.transport, indicators);
}

static int profile_listener(const zmk_event_t *eh) {
    if (as_zmk_endpoint_changed(eh)) {
        raise_led_changed_event(NULL);
    } else if (as_zmk_split_peripheral_status_changed(eh)) {
        k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &hid_indicators_send_state_work);
    }
    return 0;
}

static ZMK_LISTENER(profile_listener, profile_listener);
static ZMK_SUBSCRIPTION(profile_listener, zmk_endpoint_changed);
static ZMK_SUBSCRIPTION(profile_listener, zmk_split_peripheral_status_changed);
