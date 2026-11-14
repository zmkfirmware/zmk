/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#define BIT(x) (1U << (x))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define ARG_UNUSED(x) ((void)(x))
#define LOG_ERR(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define LOG_DBG(...) ((void)0)
#define K_MSEC(ms) (ms)
#define LED_MATRIX_TICK_MS 50
#define LED_MATRIX_SERVICE_SYNC_OUTPUT BIT(0)
#define LED_MATRIX_SERVICE_RENDER BIT(1)
#define LED_MATRIX_SERVICE_RETRY_OUTPUT BIT(2)
#include "src/led_matrix/internal.h"
struct k_work {
    void (*handler)(struct k_work *);
    bool pending;
};
struct k_work_delayable {
    struct k_work work;
    int delay;
    bool scheduled;
};
struct k_timer {
    bool running;
};
struct k_work_sync {
    int unused;
};
struct k_spinlock {
    bool locked;
};
typedef int k_spinlock_key_t;
static k_spinlock_key_t k_spin_lock(struct k_spinlock *lock) {
    assert(!lock->locked);
    lock->locked = true;
    return 0;
}
static void k_spin_unlock(struct k_spinlock *lock, k_spinlock_key_t key) {
    (void)key;
    assert(lock->locked);
    lock->locked = false;
}
typedef int k_timeout_t;
typedef int atomic_val_t;
typedef int atomic_t;
#define K_WORK_DEFINE(name, cb) struct k_work name = {.handler = cb}
#define K_WORK_DELAYABLE_DEFINE(name, cb) struct k_work_delayable name = {.work.handler = cb}
#define K_TIMER_DEFINE(name, cb, unused) struct k_timer name
static int atomic_get(atomic_t *p) { return *p; }
static void atomic_set(atomic_t *p, int value) { *p = value; }
static int atomic_clear(atomic_t *p) {
    int old = *p;
    *p = 0;
    return old;
}
static int atomic_inc(atomic_t *p) { return (*p)++; }
static void atomic_or(atomic_t *p, int value) { *p |= value; }
static int zmk_workqueue_lowprio_work_q(void) { return 1; }
static int k_work_submit_to_queue(int q, struct k_work *work) {
    assert(q == 1);
    work->pending = true;
    return 1;
}
static int k_work_reschedule_for_queue(int q, struct k_work_delayable *work, int delay) {
    assert(q == 1);
    work->delay = delay;
    work->scheduled = true;
    return 1;
}
static int k_work_cancel_delayable(struct k_work_delayable *work) {
    work->scheduled = false;
    work->work.pending = false;
    return 0;
}
static int k_work_cancel_delayable_sync(struct k_work_delayable *work, struct k_work_sync *sync) {
    (void)sync;
    return k_work_cancel_delayable(work);
}
static int k_work_cancel_sync(struct k_work *work, struct k_work_sync *sync) {
    (void)sync;
    work->pending = false;
    return 0;
}
static uint32_t k_uptime_get_32(void) { return 0; }
static void k_timer_start(struct k_timer *timer, int initial, int period) {
    assert(initial == 50 && period == 50);
    timer->running = true;
}
static void k_timer_stop(struct k_timer *timer) { timer->running = false; }
static bool enabled, sleeping, powered_down, dynamic, frame_nonzero;
static int clear_error, power_error, ext_error, render_error, sync_calls, clear_calls, render_calls;
static int ext_enable_error, ext_calls;
static bool ext_enabled;
static bool change_target_during_sync;
static atomic_t led_matrix_service_requests, led_matrix_output_generation,
    led_matrix_output_sync_failed, led_matrix_demand_generation;
static struct led_matrix_render_demand current_render_demand;
static bool led_matrix_tick_running;
extern struct k_work led_matrix_render_work;
extern struct k_work_delayable led_matrix_output_retry_work;
extern struct k_timer led_matrix_tick;
static void led_matrix_sync_tick_state(void);
static void led_matrix_queue_service_work(uint32_t requests);
static bool led_matrix_render_enabled(void) { return enabled && !sleeping; }
static bool led_matrix_is_sleep_suspended(void) { return sleeping; }
static enum led_matrix_runtime_mode led_matrix_current_runtime_mode(void) {
    return sleeping  ? LED_MATRIX_RUNTIME_MODE_SLEEP_SUSPENDED
           : enabled ? LED_MATRIX_RUNTIME_MODE_ACTIVE
                     : LED_MATRIX_RUNTIME_MODE_OFF;
}
static void led_matrix_note_low_power_clear_only(enum led_matrix_runtime_mode mode) { (void)mode; }
static bool led_matrix_runtime_output_is_quiesced(void) { return powered_down; }
static void led_matrix_invalidate_flushed_frame(void) {}
static int led_matrix_flush_cleared_output(void) {
    clear_calls++;
    return clear_error;
}
static int led_matrix_sync_backend_power(bool should_enable) {
    sync_calls++;
    if (power_error == 0) {
        powered_down = !should_enable;
    }
    if (change_target_during_sync) {
        change_target_during_sync = false;
        enabled = true;
        power_error = 0;
        led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT);
        return -EIO;
    }
    return power_error;
}
static int led_matrix_sync_ext_power(bool should_enable) {
    ext_calls++;
    ext_enabled = should_enable;
    return should_enable && ext_enable_error != 0 ? ext_enable_error : ext_error;
}
#include "active_power.inc"
static struct led_matrix_render_demand compose_render_frame(uint32_t now) {
    (void)now;
    return (struct led_matrix_render_demand){
        .nonzero = led_matrix_render_enabled() && frame_nonzero,
        .periodic = led_matrix_render_enabled() && dynamic,
    };
}
static int flush_render_frame(void) {
    /* A retry clears its failure gate only after the first frame succeeds. */
    assert(!powered_down);
    render_calls++;
    return render_error;
}
static void led_matrix_clear_runtime_suspend_state(bool sync) { assert(sync); }
static int led_matrix_raise_state_changed(uint8_t mask) {
    (void)mask;
    return 0;
}
#include "shutdown.inc"
#include "service.inc"
#include "sleep.inc"
static void drain(void) {
    int guard = 20;
    while (led_matrix_render_work.pending) {
        assert(--guard);
        led_matrix_render_work.pending = false;
        led_matrix_render_work.handler(&led_matrix_render_work);
    }
}
static void request(bool on) {
    enabled = on;
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER);
    drain();
}
static void retry(int delay) {
    assert(led_matrix_output_retry_work.scheduled && led_matrix_output_retry_work.delay == delay);
    led_matrix_output_retry_work.scheduled = false;
    led_matrix_output_retry_work.work.handler(&led_matrix_output_retry_work.work);
    drain();
}
static void reset(void) {
    sleeping = enabled = powered_down = change_target_during_sync = false;
    dynamic = true;
    frame_nonzero = true;
    clear_error = power_error = ext_error = render_error = sync_calls = clear_calls = render_calls =
        0;
    ext_enable_error = ext_calls = 0;
    ext_enabled = false;
    led_matrix_service_requests = led_matrix_output_generation = led_matrix_output_sync_failed = 0;
    led_matrix_demand_generation = 0;
    current_render_demand = (struct led_matrix_render_demand){0};
    led_matrix_tick_running = led_matrix_tick.running = false;
    led_matrix_render_work.pending = false;
    led_matrix_reset_output_retry();
}
int main(void) {
    reset();
    power_error = -EIO;
    request(false);
    retry(100);
    retry(500);
    retry(2000);
    assert(sync_calls == 4 && !led_matrix_output_retry_work.scheduled && !led_matrix_tick.running);
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_RENDER);
    drain();
    assert(sync_calls == 4 && render_calls == 0);

    reset();
    clear_error = -EIO;
    request(false);
    assert(powered_down && clear_calls == 1 && !led_matrix_output_retry_work.scheduled);
    request(false);
    assert(clear_calls == 1); /* No writes to a shut-down chip. */

    reset();
    powered_down = true;
    power_error = -EIO;
    request(true);
    assert(!led_matrix_tick.running && render_calls == 0);
    power_error = 0;
    retry(100);
    assert(render_calls == 1 && led_matrix_tick.running && !led_matrix_output_retry_work.scheduled);

    reset();
    power_error = -EIO;
    request(false);
    power_error = 0;
    request(true);
    assert(!led_matrix_output_retry_work.scheduled && !powered_down);
    int calls = sync_calls;
    led_matrix_output_retry_handler(&led_matrix_output_retry_work.work);
    drain();
    assert(sync_calls == calls && !powered_down); /* Already submitted stale retry. */

    reset();
    power_error = -EIO;
    request(true);
    power_error = 0;
    request(false);
    assert(powered_down && !led_matrix_tick.running && !led_matrix_output_retry_work.scheduled);

    reset();
    change_target_during_sync = true;
    request(false);
    assert(enabled && !powered_down && !led_matrix_output_retry_work.scheduled);

    reset();
    power_error = -EIO;
    request(false);
    sleeping = true;
    assert(led_matrix_suspend_runtime_output_sync(0) == -EIO);
    assert(!led_matrix_output_retry_work.scheduled && !led_matrix_render_work.pending);
    calls = sync_calls;
    led_matrix_output_retry_handler(&led_matrix_output_retry_work.work);
    drain();
    assert(sync_calls == calls);

    reset();
    ext_error = -EIO;
    request(false);
    assert(led_matrix_output_retry_work.scheduled);
    ext_error = 0;
    retry(100);
    assert(!led_matrix_output_retry_work.scheduled);

    /* A partial strip-rail enable must be undone before retrying the first frame. */
    reset();
    ext_enable_error = -EIO;
    request(true);
    assert(ext_calls == 2 && !ext_enabled && render_calls == 0 && !led_matrix_tick.running);
    retry(100);
    retry(500);
    retry(2000);
    assert(ext_calls == 8 && !ext_enabled && !led_matrix_output_retry_work.scheduled);
    ext_enable_error = 0;
    led_matrix_schedule_render();
    drain();
    assert(ext_enabled && render_calls == 1 && led_matrix_tick.running);

    /* Logically enabled, static black: no tick, no supply, no needless flush on repeated events. */
    reset();
    dynamic = frame_nonzero = false;
    request(true);
    assert(enabled && powered_down && !led_matrix_tick.running && render_calls == 0);
    calls = clear_calls;
    led_matrix_schedule_render();
    drain();
    assert(clear_calls == calls);
    frame_nonzero = true;
    led_matrix_schedule_render();
    drain();
    assert(enabled && !powered_down && !led_matrix_tick.running && render_calls == 1);
    frame_nonzero = false;
    led_matrix_schedule_render();
    drain();
    assert(enabled && powered_down && !led_matrix_tick.running);

    /* An animation's black phase retains power; its final black frame releases it. */
    reset();
    frame_nonzero = false;
    request(true);
    assert(!powered_down && led_matrix_tick.running);
    dynamic = false;
    led_matrix_tick_handler(&led_matrix_tick);
    drain();
    assert(enabled && powered_down && !led_matrix_tick.running);

    /* First-frame failure also rolls back and uses the same bounded retry budget. */
    reset();
    render_error = -EIO;
    request(true);
    assert(powered_down && !led_matrix_tick.running);
    retry(100);
    retry(500);
    retry(2000);
    assert(render_calls == 4 && !led_matrix_output_retry_work.scheduled);
    render_error = 0;
    led_matrix_schedule_render();
    drain();
    assert(!powered_down && led_matrix_tick.running);
    puts("PASS service: finite retry delays, error gating, clear failure fallback, stale requests, "
         "sleep cancellation, black-output gating, event wake, failed-frame rollback");
    return 0;
}
