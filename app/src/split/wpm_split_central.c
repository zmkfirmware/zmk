/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/split_wpm_state_changed.h>
//#include <zmk/split/bluetooth/central.h>
#include <zmk/wpm.h>

// Chỉ build file này cho CENTRAL
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

static int wpm_state_changed_listener(const zmk_event_t *eh) {
    // Lấy WPM hiện tại
    uint8_t wpm = zmk_wpm_get_state();
    
    LOG_DBG("Broadcasting WPM to peripherals: %d", wpm);
    
    // Tạo event để broadcast
    struct zmk_split_wpm_state_changed ev = {
        .wpm = wpm
    };
    
    // Raise event - ZMK sẽ tự động broadcast đến peripherals
    return ZMK_EV_EVENT_BUBBLE(&ev);
}

ZMK_LISTENER(wpm_split_central, wpm_state_changed_listener);
ZMK_SUBSCRIPTION(wpm_split_central, zmk_wpm_state_changed);

#endif // CONFIG_ZMK_SPLIT_ROLE_CENTRAL
