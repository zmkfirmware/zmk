#pragma once

#include <zephyr.h>
#include <zmk/event-manager.h>

struct position_state_changed {
    struct zmk_event_header header;
    u32_t position;
    bool state;
};

ZMK_EVENT_DECLARE(position_state_changed);