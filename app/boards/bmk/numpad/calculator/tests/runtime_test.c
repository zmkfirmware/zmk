/* SPDX-License-Identifier: MIT */
/* Compile the real runtime against a deterministic, single-threaded ZMK shim. */
#include "fake_zmk.h"
#include "../sender.c"
#include "../calculator.c"
#include "../behavior.c"
#include <assert.h>

static struct {
    uint32_t code;
    bool pressed;
} reports[256];
static size_t report_count;
static unsigned layers;
static bool connected = true;
static unsigned modifiers;
static uint32_t physical_key;
static struct zmk_endpoint_instance endpoint = {1, 0};

bool zmk_endpoint_is_connected(void) { return connected; }
struct zmk_endpoint_instance zmk_endpoint_get_selected(void) { return endpoint; }
bool zmk_endpoint_instance_eq(struct zmk_endpoint_instance a, struct zmk_endpoint_instance b) {
    return a.transport == b.transport && a.profile == b.profile;
}
bool zmk_hid_mod_is_pressed(int i) { return modifiers & (1U << i); }
bool zmk_hid_is_pressed(uint32_t code) { return code == physical_key; }
int raise_zmk_keycode_state_changed_from_encoded(uint32_t code, bool down, int64_t timestamp) {
    assert(report_count < ARRAY_SIZE(reports));
    reports[report_count].code = code;
    reports[report_count++].pressed = down;
    return 0;
}
bool zmk_keymap_layer_active(uint8_t layer) { return layers & (1U << layer); }
static int set_layer(uint8_t layer, bool on) {
    if (on) {
        layers |= 1U << layer;
    } else {
        layers &= ~(1U << layer);
    }
    struct zmk_layer_state_changed ev = {.layer = layer, .state = on};
    return layer_changed((const zmk_event_t *)&ev);
}
int zmk_keymap_layer_activate(uint8_t layer, bool locking) { return set_layer(layer, true); }
int zmk_keymap_layer_deactivate(uint8_t layer, bool locking) { return set_layer(layer, false); }
static void key(uint32_t code) {
    struct zmk_behavior_binding binding = {.param1 = code};
    struct zmk_behavior_binding_event ev = {0};
    pressed(&binding, ev);
    released(&binding, ev);
}
static void drain(void) {
    for (int i = 0; i < 100 && busy; i++) {
        send_work(NULL);
    }
    assert(!busy && held == 0);
}
static void pairs(void) {
    assert(report_count % 2 == 0);
    for (size_t i = 0; i < report_count; i += 2) {
        assert(reports[i].pressed && !reports[i + 1].pressed);
        assert(reports[i].code == reports[i + 1].code);
    }
}
int main(void) {
    assert(!numpad_calculator_snapshot().active);
    layers = 1U << 1; /* Media is preserved underneath calculator layers. */
    key(CALC_TOGGLE);
    assert(numpad_calculator_snapshot().active && zmk_keymap_layer_active(1));
    key(KP_N2);
    key(KP_PLUS);
    key(KP_N3);
    key(KP_MULTIPLY);
    key(KP_N4);
    key(KP_ENTER);
    assert(!strcmp(numpad_calculator_snapshot().engine.result, "14"));
    assert(report_count == 0); /* Local input and releases never emit HID. */
    zmk_keymap_layer_activate(4, false);
    key(CALC_TOGGLE);
    assert(layers == (1U << 1));
    assert(!numpad_calculator_snapshot().active);
    key(KP_N9); /* Release/old bindings after exit cannot alter the expression. */
    key(CALC_TOGGLE);
    assert(!strcmp(numpad_calculator_snapshot().engine.result, "14"));
    key(CALC_SEND);
    assert(numpad_calculator_send("99") == -EBUSY);
    drain();
    pairs();
    assert(report_count == 4 && reports[0].code == N1 && reports[2].code == N4);
    assert(!strcmp(numpad_calculator_snapshot().notice, "Sent"));
    report_count = 0;
    numpad_calculator_send("-1.25e12");
    drain();
    pairs();
    assert(report_count == 16 && reports[0].code == MINUS && reports[4].code == DOT);
    report_count = 0;
    connected = false;
    key(CALC_SEND);
    assert(!busy && !report_count);
    assert(!strcmp(numpad_calculator_snapshot().notice, "Not connected"));
    connected = true;
    modifiers = 1;
    assert(numpad_calculator_send("12") == -EBUSY);
    modifiers = 0;
    numpad_calculator_send("123");
    send_work(NULL);
    assert(held);
    endpoint.profile = 1;
    endpoint_changed(NULL);
    drain();
    pairs();
    assert(report_count == 2);
    report_count = 0;
    numpad_calculator_send("123");
    send_work(NULL);
    connected = false;
    drain();
    pairs();
    assert(report_count == 2);
    connected = true;
    report_count = 0;
    numpad_calculator_send("123");
    endpoint_changed(NULL); /* Even a rapid change back cancels the pending send. */
    drain();
    assert(report_count == 0);
    numpad_calculator_send("123");
    physical_key = N1;
    drain();
    assert(report_count == 0);
    physical_key = 0;
    numpad_calculator_send("123");
    send_work(NULL);
    key(CALC_TOGGLE);
    drain();
    pairs();
    assert(report_count == 2);
    key(CALC_TOGGLE);
    key(DEL);
    key(KP_N1);
    key(KP_DIVIDE);
    key(KP_N0);
    report_count = 0;
    key(CALC_SEND);
    assert(!busy && report_count == 0 &&
           numpad_calculator_snapshot().engine.error == CALC_DIV_ZERO);
    puts("calculator runtime / HID sender tests passed");
}
