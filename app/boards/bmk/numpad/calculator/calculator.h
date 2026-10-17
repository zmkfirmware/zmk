/* SPDX-License-Identifier: MIT */
#pragma once
#include "engine.h"
#include <zmk/event_manager.h>

struct numpad_calculator_changed {
    bool active;
};
ZMK_EVENT_DECLARE(numpad_calculator_changed);

struct numpad_calculator_state {
    struct calc_engine engine;
    bool active;
    char notice[32];
};

struct numpad_calculator_state numpad_calculator_snapshot(void);
void numpad_calculator_notify(const char *notice);
void numpad_calculator_key(uint32_t key);
int numpad_calculator_send(const char *text);
void numpad_calculator_send_cancel(void);
