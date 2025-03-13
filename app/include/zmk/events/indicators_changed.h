#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_indicators_battery_status_asked {
    uint8_t level;
};
ZMK_EVENT_DECLARE(zmk_indicators_battery_status_asked);

struct zmk_indicators_state_changed {
    uint8_t state;
};
ZMK_EVENT_DECLARE(zmk_indicators_state_changed);