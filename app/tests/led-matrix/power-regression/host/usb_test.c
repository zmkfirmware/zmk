/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE 1
#define IS_ENABLED(x) (x)
#define LOG_DBG(...) ((void)0)
#define LED_MATRIX_STATUS_USB 2
#define LED_MATRIX_STATUS_BATTERY 1
#define ZMK_LED_MATRIX_STATE_CHANGE_ENABLED 1
#define ZMK_LED_MATRIX_STATE_CHANGE_STATUS 2
#define ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR 4
#define LED_MATRIX_WAKE_CONTEXT_NONE 0
#define ZMK_ACTIVITY_IDLE 1
#define ZMK_ACTIVITY_ACTIVE 0
#define ZMK_USB_CONN_NONE 0
#define ZMK_USB_CONN_POWERED 1
#define ZMK_USB_CONN_HID 2
struct led_matrix_snapshot {
    uint8_t usb_state, battery_soc;
    int output_transport;
};
struct zmk_usb_conn_state_changed {
    uint8_t conn_state;
};
static struct led_matrix_snapshot snapshot;
static struct {
    bool on;
} state;
static struct {
    bool suspended, restore_on_wake;
} idle_auto_off_state;
static bool sleeping, force_show_active;
static int activity, force_show_wake_context, scheduled, status_events, transport_events;
static int led_matrix_current_battery_soc(void) { return 50; }
static int led_matrix_current_usb_state(void) { return ZMK_USB_CONN_NONE; }
static int led_matrix_current_output_transport(void) { return 0; }
static int zmk_activity_get_state(void) { return activity; }
static bool led_matrix_is_auto_suspended(void) { return idle_auto_off_state.suspended; }
static bool led_matrix_is_sleep_suspended(void) { return sleeping; }
static void led_matrix_cancel_idle_auto_off(void) { scheduled = 0; }
static void led_matrix_schedule_idle_auto_off_if_needed(void) { scheduled++; }
static void led_matrix_reset_auto_suspend_state(void) {
    idle_auto_off_state.suspended = idle_auto_off_state.restore_on_wake = false;
    led_matrix_cancel_idle_auto_off();
}
static int led_matrix_suspend_runtime_output_async(uint8_t mask) {
    (void)mask;
    force_show_active = false;
    return 0;
}
static int led_matrix_resume_runtime_output(uint8_t mask) {
    (void)mask;
    return 0;
}
static bool same_status(int source, const struct led_matrix_snapshot *updated) {
    assert(source == LED_MATRIX_STATUS_USB);
    return updated->usb_state == snapshot.usb_state;
}
static void led_matrix_show_auto_status_sources(uint8_t sources, bool merge) {
    assert(merge && (sources & LED_MATRIX_STATUS_USB));
    /* Observe policy BEFORE overlay processing: the production status machinery is tested by
     * native_sim. */
    if (activity == ZMK_ACTIVITY_IDLE && snapshot.usb_state == ZMK_USB_CONN_NONE && state.on &&
        !sleeping) {
        assert(idle_auto_off_state.suspended);
    }
    status_events++;
}
static void led_matrix_handle_output_transport_change(int previous, int updated) {
    (void)previous;
    (void)updated;
    transport_events++;
}
#include "usb_policy.inc"
#include "usb.inc"
static void event(int usb) {
    struct zmk_usb_conn_state_changed ev = {.conn_state = usb};
    led_matrix_handle_usb_status_event(&ev);
}
static void reset(void) {
    snapshot = (struct led_matrix_snapshot){.usb_state = ZMK_USB_CONN_POWERED};
    state.on = true;
    sleeping = force_show_active = false;
    activity = ZMK_ACTIVITY_IDLE;
    idle_auto_off_state.suspended = idle_auto_off_state.restore_on_wake = false;
    scheduled = status_events = transport_events = 0;
}
int main(void) {
    reset();
    /* Supply getter deliberately says NONE; policy must use the event snapshot. */
    assert(led_matrix_auto_suspend() == 0 && !idle_auto_off_state.suspended);
    for (int i = 0; i < 100; i++) {
        event(ZMK_USB_CONN_NONE);
        assert(idle_auto_off_state.suspended && state.on);
        event(ZMK_USB_CONN_POWERED);
        assert(!idle_auto_off_state.suspended && state.on && scheduled == 0);
    }
    reset();
    state.on = false;
    event(ZMK_USB_CONN_NONE);
    event(ZMK_USB_CONN_POWERED);
    assert(!state.on && !idle_auto_off_state.suspended);
    reset();
    sleeping = true;
    event(ZMK_USB_CONN_NONE);
    event(ZMK_USB_CONN_POWERED);
    assert(sleeping && !idle_auto_off_state.suspended);
    reset();
    activity = ZMK_ACTIVITY_ACTIVE;
    event(ZMK_USB_CONN_NONE);
    assert(!idle_auto_off_state.suspended);
    activity = ZMK_ACTIVITY_IDLE;
    assert(led_matrix_auto_suspend() == 0 && idle_auto_off_state.suspended);
    reset();
    event(ZMK_USB_CONN_HID);
    assert(!idle_auto_off_state.suspended && status_events == 1);
    event(ZMK_USB_CONN_HID);
    assert(status_events == 1); /* Duplicate event doesn't reset policy/timers. */
    puts("PASS USB: 100 idle unplug/replug cycles, latest event state, manual off, sleep, active "
         "unplug");
    return 0;
}
