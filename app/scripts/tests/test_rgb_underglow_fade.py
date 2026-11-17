# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT
"""Host regression checks for the actual RGB underglow source.

Run: python3 app/scripts/tests/test_rgb_underglow_fade.py
The mocked kernel and devices exercise transitions without a Zephyr toolchain.
"""
from pathlib import Path
import re, subprocess, tempfile

source = Path(__file__).resolve().parents[2] / "src/rgb_underglow.c"
MOCKS = r"""

#ifndef RGB_FADE_TEST_MOCKS
#define RGB_FADE_TEST_MOCKS
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#define IS_ENABLED(x) (x)
#define CONFIG_SETTINGS 0
#define CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE 1
#define CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB 1
#define CONFIG_ZMK_LOG_LEVEL 0
#define CONFIG_APPLICATION_INIT_PRIORITY 0
#define CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN 0
#define CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX 100
#define CONFIG_ZMK_RGB_UNDERGLOW_HUE_START 0
#define CONFIG_ZMK_RGB_UNDERGLOW_SAT_START 100
#define CONFIG_ZMK_RGB_UNDERGLOW_BRT_START 100
#define CONFIG_ZMK_RGB_UNDERGLOW_SPD_START 1
#define CONFIG_ZMK_RGB_UNDERGLOW_EFF_START 0
#define CONFIG_ZMK_RGB_UNDERGLOW_ON_START 1
#define CONFIG_ZMK_RGB_UNDERGLOW_HUE_STEP 10
#define CONFIG_ZMK_RGB_UNDERGLOW_SAT_STEP 10
#define CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP 10
#define BUILD_ASSERT(c, m) _Static_assert(c, m)
#define DT_HAS_CHOSEN(x) 1
#define DT_CHOSEN(x) 0
#define DT_PROP(x, p) 2
#define DT_INST(x, y) 0
#define DEVICE_DT_GET(x) (&device)
#define LOG_MODULE_DECLARE(...)
#define LOG_ERR(...)
#define SYS_INIT(...)
#define ZMK_LISTENER(...)
#define ZMK_SUBSCRIPTION(...)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define K_FOREVER -1
#define K_NO_WAIT 0
#define K_MSEC(x) (x)
#define ZMK_ACTIVITY_ACTIVE 1
struct device {
    int unused;
};
static const struct device device;
static bool powered;
static int writes, disable_calls, schedule_result, next_delay;
static int64_t now;
struct led_rgb {
    uint8_t r, g, b;
};
struct zmk_led_hsb {
    uint16_t h;
    uint8_t s, b;
};
static int led_strip_update_rgb(const struct device *d, struct led_rgb *p, size_t n) {
    writes++;
    return 0;
}
static int ext_power_enable(const struct device *d) {
    powered = true;
    return 0;
}
static int ext_power_disable(const struct device *d) {
    powered = false;
    disable_calls++;
    return 0;
}
static bool device_is_ready(const struct device *d) { return true; }
struct k_work {
    void (*handler)(struct k_work *);
};
struct k_work_delayable {
    struct k_work work;
    bool pending;
};
struct k_timer {
    bool active;
};
struct k_mutex {
    int held;
};
#define K_MUTEX_DEFINE(name) struct k_mutex name
static int k_mutex_lock(struct k_mutex *m, int timeout) {
    assert(!m->held);
    m->held = 1;
    return 0;
}
static void k_mutex_unlock(struct k_mutex *m) {
    assert(m->held);
    m->held = 0;
}
#define K_WORK_DEFINE(name, fn) struct k_work name = {.handler = fn}
#define K_WORK_DELAYABLE_DEFINE(name, fn) struct k_work_delayable name = {.work = {.handler = fn}}
#define K_TIMER_DEFINE(name, fn, stop) struct k_timer name
static void k_timer_start(struct k_timer *t, int delay, int period) { t->active = true; }
static void k_timer_stop(struct k_timer *t) { t->active = false; }
static int k_work_submit_to_queue(void *q, struct k_work *w) {
    w->handler(w);
    return 0;
}
static int k_work_reschedule_for_queue(void *q, struct k_work_delayable *w, int delay) {
    if (schedule_result < 0)
        return schedule_result;
    w->pending = true;
    next_delay = delay;
    return 0;
}
static int k_work_cancel_delayable(struct k_work_delayable *w) {
    w->pending = false;
    return 0;
}
static void *zmk_workqueue_lowprio_work_q(void) { return 0; }
static int64_t k_uptime_get(void) { return now; }
static bool usb_powered = true;
static bool activity_active = true;
static bool zmk_usb_is_powered(void) { return usb_powered; }
static int zmk_activity_get_state(void) { return activity_active; }
typedef struct {
    int type;
} zmk_event_t;
static void *as_zmk_activity_state_changed(const zmk_event_t *e) {
    return e->type == 1 ? (void *)e : 0;
}
static void *as_zmk_usb_conn_state_changed(const zmk_event_t *e) {
    return e->type == 2 ? (void *)e : 0;
}
int zmk_rgb_underglow_on(void);
int zmk_rgb_underglow_off(void);
#endif
"""
TESTS = r"""

static void render_fixture(void) {
    pixels[0] = (struct led_rgb){240, 120, 60};
    pixels[1] = (struct led_rgb){0, 60, 180};
}
#if CONFIG_ZMK_RGB_UNDERGLOW_FADE_OFF
static void run_fade(void) {
    underglow_fade_work.pending = false;
    underglow_fade_work.work.handler(&underglow_fade_work.work);
    assert(!underglow_mutex.held);
}
#endif
int main(void) {
    assert(zmk_rgb_underglow_init() == 0);
    state.color = (struct zmk_led_hsb){123, 45, 67};
    state.current_effect = UNDERGLOW_EFFECT_SWIRL;
    struct zmk_led_hsb selected = state.color;
    assert(zmk_rgb_underglow_on() == 0);
    render_fixture();
    assert(zmk_rgb_underglow_off() == 0);
    bool logical_on = true;
    assert(zmk_rgb_underglow_get_state(&logical_on) == 0 && !logical_on);
#if CONFIG_ZMK_RGB_UNDERGLOW_FADE_OFF
    assert(underglow_fade_active);
    assert(disable_calls == 0);
    int before = writes;
    zmk_rgb_underglow_tick(&underglow_tick_work); // A queued animation tick cannot repaint.
    assert(writes == before && pixels[0].r == 240);
    run_fade();
    now = CONFIG_ZMK_RGB_UNDERGLOW_FADE_OFF_MS / 2;
    run_fade();
    assert(pixels[0].r < 240 && pixels[0].r > 0 && pixels[0].g < pixels[0].r);
    assert(pixels[1].r == 0 && pixels[1].b > pixels[1].g);
    assert(disable_calls == 0);
    if (CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
        assert(powered);
    int64_t started = underglow_fade_start;
    zmk_rgb_underglow_off(); // Repeated off must not reset its deadline.
    assert(underglow_fade_start == started);
    now = CONFIG_ZMK_RGB_UNDERGLOW_FADE_OFF_MS + 10;
    run_fade();
    assert(disable_calls == CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER);
    assert(!underglow_fade_active);
#else
    assert(disable_calls == CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER);
#endif
    for (int i = 0; i < STRIP_NUM_PIXELS; i++)
        assert(!pixels[i].r && !pixels[i].g && !pixels[i].b);
    assert(state.color.h == selected.h && state.color.s == selected.s &&
           state.color.b == selected.b);
    assert(state.current_effect == UNDERGLOW_EFFECT_SWIRL);
    // An explicit off still clears stale output when the logical state was already off.
    render_fixture();
    powered = true;
    zmk_rgb_underglow_off();
    assert(!pixels[0].r);
    if (CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
        assert(!powered);
#if CONFIG_ZMK_RGB_UNDERGLOW_FADE_OFF
    zmk_rgb_underglow_on();
    render_fixture();
    now = 0;
    zmk_rgb_underglow_off();
    run_fade();
    now = 10;
    run_fade();
    int disables_before = disable_calls;
    zmk_rgb_underglow_on();
    int writes_before = writes;
    now = CONFIG_ZMK_RGB_UNDERGLOW_FADE_OFF_MS + 1;
    run_fade(); // Even a stale callback cannot blank or depower newly enabled lights.
    assert(writes == writes_before && disable_calls == disables_before && state.on);
    if (CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
        assert(powered);
    schedule_result = -EIO;
    zmk_rgb_underglow_off();
    assert(!underglow_fade_active && !pixels[0].r);
    if (CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
        assert(!powered);
    schedule_result = 0;
    zmk_rgb_underglow_on();
    render_fixture();
    now = 0;
    zmk_rgb_underglow_off();
    schedule_result = -EIO;
    run_fade(); // Rescheduling failure also finishes off safely.
    assert(!underglow_fade_active && !pixels[0].r);
    schedule_result = 0;
#endif
    zmk_rgb_underglow_on();
    zmk_event_t event = {.type = 2};
    usb_powered = false;
    rgb_underglow_event_listener(&event);
    assert(!state.on);
    usb_powered = true;
    rgb_underglow_event_listener(&event);
    assert(state.on);
    led_strip = NULL;
    assert(zmk_rgb_underglow_on() == -ENODEV && zmk_rgb_underglow_off() == -ENODEV);
    return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="zmk-upstream-fade-") as directory:
    root = Path(directory)
    (root / "mocks.h").write_text(MOCKS)
    for header in re.findall(r"#include <([^>]+)>", source.read_text()):
        if header in ["math.h", "stdlib.h", "string.h"]:
            continue
        p = root / header
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('#include "mocks.h"\n')
    harness = root / "test.c"
    harness.write_text('#include "' + str(source) + '"\n' + TESTS)
    compiler_flags = (
        ["-Wno-gnu-designator"]
        if "clang" in subprocess.check_output(["cc", "--version"], text=True).lower()
        else []
    )
    for fade in [0, 1]:
        for power in [0, 1]:
            for duration in [25, 37, 1500, 10000]:
                binary = root / "test"
                subprocess.run(
                    [
                        "cc",
                        "-std=c11",
                        "-Wall",
                        "-Wextra",
                        "-Werror",
                        "-Wno-unused-parameter",
                        "-Wno-unused-function",
                        "-Wno-unused-variable",
                        *compiler_flags,
                        f"-DCONFIG_ZMK_RGB_UNDERGLOW_FADE_OFF={fade}",
                        f"-DCONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER={power}",
                        f"-DCONFIG_ZMK_RGB_UNDERGLOW_FADE_OFF_MS={duration}",
                        "-I",
                        str(root),
                        str(harness),
                        "-o",
                        str(binary),
                    ],
                    check=True,
                )
                subprocess.run([str(binary)], check=True)
print(
    "PASS: 16 full-source C configurations; fade endpoints, colors/settings, queued ticks, repeated off, wake cancellation, scheduling failures, power gating, legacy behavior, and USB auto-off"
)
