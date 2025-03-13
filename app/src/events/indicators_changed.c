#include <zephyr/kernel.h>
#include <zmk/events/indicators_changed.h>

ZMK_EVENT_IMPL(zmk_indicators_battery_status_asked);
ZMK_EVENT_IMPL(zmk_indicators_state_changed);