/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/*
 * A split peripheral has no endpoints, so it cannot keep the state per profile the
 * way hid_indicators.c does. It only caches what the central last reported for the
 * endpoint the central itself has selected, which is what consumers such as
 * indicator LEDs ask for anyway.
 */
static zmk_hid_indicators_t hid_indicators;

zmk_hid_indicators_t zmk_hid_indicators_get_current_profile(void) { return hid_indicators; }

int zmk_hid_indicators_set_mirrored_state(zmk_hid_indicators_t indicators) {
    if (hid_indicators == indicators) {
        return 0;
    }

    hid_indicators = indicators;

    LOG_DBG("Update HID indicators: indicators=%x", indicators);

    return raise_zmk_hid_indicators_changed(
        (struct zmk_hid_indicators_changed){.indicators = indicators});
}
