/* SPDX-License-Identifier: MIT */
#include "fake_zmk.h"
#undef CONFIG_NUMPAD_CALCULATOR
#define CONFIG_NUMPAD_CALCULATOR 0
#include "../behavior.c"
#include <assert.h>

int main(void) {
    struct zmk_behavior_binding binding = {.param1 = CALC_TOGGLE};
    struct zmk_behavior_binding_event event = {0};
    assert(pressed(&binding, event) == ZMK_BEHAVIOR_OPAQUE);
    assert(released(&binding, event) == ZMK_BEHAVIOR_OPAQUE);
    puts("calculator disabled behavior tests passed");
}
