#pragma once

#include <zephyr.h>
#include <zmk/event-manager.h>

struct keycode_state_changed {
    struct zmk_event_header header;
    u8_t usage_page;
    u32_t keycode;
    bool state;
};

ZMK_EVENT_DECLARE(keycode_state_changed);