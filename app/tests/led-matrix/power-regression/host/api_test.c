/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define BIT(x) (1U << (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CONFIG_SETTINGS 1
#define CONFIG_ZMK_LED_MATRIX_BRT_STEP 10
#define CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE 5000
#define IS_ENABLED(x) (x)
#define LOG_ERR(...) ((void)0)
#define LOG_DBG(...) ((void)0)
#define K_MSEC(ms) (ms)
#define LED_MATRIX_SERVICE_SYNC_OUTPUT BIT(0)
#define LED_MATRIX_SERVICE_RENDER BIT(1)
#define ZMK_LED_MATRIX_STATE_CHANGE_ENABLED BIT(0)
#define ZMK_LED_MATRIX_STATE_CHANGE_BRIGHTNESS BIT(1)
#define ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR BIT(3)
#define ZMK_LED_MATRIX_STATE_CHANGE_STATUS BIT(4)
#define LED_MATRIX_WAKE_CONTEXT_NONE 0

struct k_work {
    bool pending;
};
struct k_work_delayable {
    int delay;
};
typedef int atomic_t;
typedef int atomic_val_t;
static struct {
    bool on;
    uint8_t brightness;
} state;
static struct {
    bool active;
} ripple_state;
static bool force_show_active, timer_running;
static int force_show_wake_context;
static atomic_t led_matrix_output_generation, led_matrix_service_requests;
static struct k_work led_matrix_render_work;
static struct k_work_delayable led_matrix_save_work;
static unsigned saves, queue_calls, cleared_status, cleared_layer, cleared_releases, reset_rain;
static unsigned auto_resets, restored_overlays, idle_schedules, refreshed_status, tick_syncs;
static int save_error;
static uint8_t last_reason;
static int atomic_inc(atomic_t *value) { return (*value)++; }
static void atomic_or(atomic_t *value, atomic_val_t bits) { *value |= bits; }
static int zmk_workqueue_lowprio_work_q(void) { return 1; }
static int k_work_submit_to_queue(int queue, struct k_work *work) {
    assert(queue == 1 && work == &led_matrix_render_work);
    queue_calls++;
    work->pending = true;
    return 1;
}
static int k_work_reschedule_for_queue(int queue, struct k_work_delayable *work, int delay) {
    assert(queue == 1 && work == &led_matrix_save_work);
    assert(delay == CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE);
    saves++;
    work->delay = delay;
    return save_error;
}
static void led_matrix_clear_transient_status_state(void) { cleared_status++; }
static void led_matrix_clear_layer_color_runtime_state(void) { cleared_layer++; }
static void led_matrix_clear_transient_status_state_sync(void) { assert(false); }
static void led_matrix_clear_layer_color_runtime_state_sync(void) { assert(false); }
static void clear_keypress_release_state(void) { cleared_releases++; }
static void led_matrix_reset_rain_state(void) { reset_rain++; }
static void led_matrix_reset_auto_suspend_state(void) { auto_resets++; }
static void led_matrix_restore_base_runtime_overlays(void) { restored_overlays++; }
static void led_matrix_schedule_idle_auto_off_if_needed(void) { idle_schedules++; }
static void led_matrix_refresh_automatic_persistent_status_sources(void) { refreshed_status++; }
static void led_matrix_sync_tick_state(void) {
    /* The real suspend entry must expose OFF before it asks the timer to stop. */
    assert(!state.on && !force_show_active);
    tick_syncs++;
    timer_running = false;
}
static int led_matrix_raise_state_changed(uint8_t reason) {
    last_reason = reason;
    return 0;
}
#include "api.inc"

static void consume_request(void) {
    led_matrix_service_requests = 0;
    led_matrix_render_work.pending = false;
}
int main(void) {
    state.on = true;
    state.brightness = 37;
    force_show_active = ripple_state.active = timer_running = true;
    force_show_wake_context = 4;
    assert(zmk_led_matrix_set_brt(0) == 0);
    assert(!zmk_led_matrix_is_on() && zmk_led_matrix_get_brt() == 0);
    assert(state.brightness == 37 && !force_show_active && !ripple_state.active && !timer_running);
    assert(force_show_wake_context == LED_MATRIX_WAKE_CONTEXT_NONE);
    assert(cleared_status == 1 && cleared_layer == 1 && cleared_releases == 1 && reset_rain == 1);
    assert(tick_syncs == 1 && refreshed_status == 1 && auto_resets == 1);
    assert(led_matrix_service_requests == LED_MATRIX_SERVICE_SYNC_OUTPUT &&
           led_matrix_render_work.pending);
    assert(queue_calls == 1 && led_matrix_output_generation == 1 && saves == 1);
    assert((last_reason & ZMK_LED_MATRIX_STATE_CHANGE_ENABLED) != 0);
    assert((last_reason & ZMK_LED_MATRIX_STATE_CHANGE_BRIGHTNESS) == 0);

    consume_request();
    assert(zmk_led_matrix_on() == 0);
    assert(zmk_led_matrix_is_on() && zmk_led_matrix_get_brt() == 37 && state.brightness == 37);
    assert(led_matrix_service_requests ==
           (LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER));
    assert(queue_calls == 2 && saves == 2 && restored_overlays == 1 && idle_schedules == 1);

    /* OFF saves the user setting once, retains brightness, and getters are read-only. */
    consume_request();
    assert(zmk_led_matrix_off() == 0);
    assert(!zmk_led_matrix_is_on() && zmk_led_matrix_get_brt() == 0 && state.brightness == 37);
    assert(saves == 3 && queue_calls == 3 && tick_syncs == 2);
    assert(!zmk_led_matrix_is_on() && zmk_led_matrix_get_brt() == 0);
    assert(saves == 3 && queue_calls == 3);

    consume_request();
    force_show_active = true;
    assert(zmk_led_matrix_set_brt(63) == 0);
    assert(zmk_led_matrix_is_on() && zmk_led_matrix_get_brt() == 63 && !force_show_active);
    assert(saves == 4 && queue_calls == 4);
    assert((last_reason & ZMK_LED_MATRIX_STATE_CHANGE_BRIGHTNESS) != 0);
    assert(zmk_led_matrix_set_brt(255) == 0 && zmk_led_matrix_get_brt() == 100);
    assert(saves == 5);

    /* The public return remains the save-scheduler result, even though output
     * shutdown was already queued. No hardware-completion guarantee is invented. */
    consume_request();
    timer_running = true;
    save_error = -ENOMEM;
    assert(zmk_led_matrix_set_brt(0) == -ENOMEM);
    assert(!state.on && state.brightness == 100 && !timer_running);
    assert(led_matrix_render_work.pending &&
           led_matrix_service_requests == LED_MATRIX_SERVICE_SYNC_OUTPUT);
    assert(saves == 6 && queue_calls == 6);
    save_error = 0;
    assert(zmk_led_matrix_on() == 0 && zmk_led_matrix_get_brt() == 100);
    assert(saves == 7);
    state.brightness = 1;
    assert(zmk_led_matrix_on() == 0 && zmk_led_matrix_get_brt() == CONFIG_ZMK_LED_MATRIX_BRT_STEP);
    assert(saves == 8);
    puts("PASS API: real brightness-zero -> OFF -> async suspend/output queue; remembered "
         "brightness, getters, ON restore and existing save semantics");
    return 0;
}
