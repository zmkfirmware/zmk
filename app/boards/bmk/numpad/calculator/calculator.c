/* SPDX-License-Identifier: MIT */
#include "calculator.h"
#include <dt-bindings/zmk/keys.h>
#include <stdio.h>
#include <string.h>
#include "include/dt-bindings/numpad-calculator.h"
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#define CALCULATOR_NODE DT_INST(0, zmk_behavior_numpad_calculator)

ZMK_EVENT_IMPL(numpad_calculator_changed);
K_MUTEX_DEFINE(calculator_mutex);
static struct numpad_calculator_state state = {.engine.result = "0"};

struct numpad_calculator_state numpad_calculator_snapshot(void) {
    k_mutex_lock(&calculator_mutex, K_FOREVER);
    struct numpad_calculator_state copy = state;
    k_mutex_unlock(&calculator_mutex);
    return copy;
}

void numpad_calculator_notify(const char *notice) {
    k_mutex_lock(&calculator_mutex, K_FOREVER);
    snprintf(state.notice, sizeof(state.notice), "%s", notice);
    bool active = state.active;
    k_mutex_unlock(&calculator_mutex);
    raise_numpad_calculator_changed((struct numpad_calculator_changed){.active = active});
}

void numpad_calculator_key(uint32_t key) {
    static const uint32_t digits[] = {KP_N0, KP_N1, KP_N2, KP_N3, KP_N4,
                                      KP_N5, KP_N6, KP_N7, KP_N8, KP_N9};
    char send[CALC_RESULT_SIZE] = {0};
    k_mutex_lock(&calculator_mutex, K_FOREVER);
    if (!state.active) {
        k_mutex_unlock(&calculator_mutex);
        return;
    }
    for (size_t i = 0; i < ARRAY_SIZE(digits); i++) {
        if (key == digits[i]) {
            calc_input(&state.engine, '0' + i);
        }
    }
    switch (key) {
    case KP_DOT:
        calc_input(&state.engine, '.');
        break;
    case KP_PLUS:
        calc_input(&state.engine, '+');
        break;
    case KP_MINUS:
        calc_input(&state.engine, '-');
        break;
    case KP_MULTIPLY:
        calc_input(&state.engine, '*');
        break;
    case KP_DIVIDE:
        calc_input(&state.engine, '/');
        break;
    case BSPC:
        calc_backspace(&state.engine);
        break;
    case DEL:
        calc_clear(&state.engine);
        break;
    case KP_ENTER:
    case KP_EQUAL:
        calc_evaluate(&state.engine);
        break;
    case CALC_SEND:
        if (calc_evaluate(&state.engine)) {
            strcpy(send, state.engine.result);
        }
        break;
    default:
        break;
    }
    k_mutex_unlock(&calculator_mutex);
    numpad_calculator_notify("");
    if (send[0]) {
        numpad_calculator_send(send);
    }
}

static int layer_changed(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev->layer != DT_PROP(CALCULATOR_NODE, calculator_layer)) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    k_mutex_lock(&calculator_mutex, K_FOREVER);
    state.active = ev->state;
    k_mutex_unlock(&calculator_mutex);
    if (!ev->state) {
        numpad_calculator_send_cancel();
    }
    numpad_calculator_notify("");
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(numpad_calculator_layer, layer_changed);
ZMK_SUBSCRIPTION(numpad_calculator_layer, zmk_layer_state_changed);
