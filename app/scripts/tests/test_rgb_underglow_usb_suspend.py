# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT
"""Host regression checks for underglow USB suspend and combined power policy.

Run: python3 app/scripts/tests/test_rgb_underglow_usb_suspend.py
"""
from pathlib import Path
import re
import subprocess
import tempfile
from itertools import product

source = Path(__file__).resolve().parents[2] / "src/rgb_underglow.c"
MOCKS = r"""


#ifndef RGB_USB_SUSPEND_TEST_MOCKS
#define RGB_USB_SUSPEND_TEST_MOCKS
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#define IS_ENABLED(x) (x)
#define CONFIG_SETTINGS TEST_SETTINGS
#define CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE 0
#define CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE TEST_IDLE
#define CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB TEST_USB
#define CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB_SUSPEND TEST_SUSPEND
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
static int writes, disable_calls;

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
};
struct k_timer {
    bool active;
};
#define K_WORK_DEFINE(name, fn) struct k_work name = {.handler = fn}
#define K_TIMER_DEFINE(name, fn, stop) struct k_timer name
static void k_timer_start(struct k_timer *t, int delay, int period) { t->active = true; }
static void k_timer_stop(struct k_timer *t) { t->active = false; }
static int k_work_submit_to_queue(void *q, struct k_work *w) {
    w->handler(w);
    return 0;
}
static void *zmk_workqueue_lowprio_work_q(void) { return 0; }

enum usb_dc_status_code { USB_DC_CONFIGURED, USB_DC_SUSPEND, USB_DC_DISCONNECTED, USB_DC_RESUME };
static enum usb_dc_status_code usb_status =
    TEST_BOOT_SUSPENDED ? USB_DC_SUSPEND : USB_DC_CONFIGURED;
static enum usb_dc_status_code zmk_usb_get_status(void) { return usb_status; }
typedef int (*settings_read_cb)(void *, void *, size_t);
#define SETTINGS_STATIC_HANDLER_DEFINE(...)
static bool settings_name_steq(const char *name, const char *key, const char **next) {
    *next = NULL;
    return strcmp(name, key) == 0;
}
static int settings_save_one(const char *name, const void *data, size_t size) { return 0; }
static void k_work_init_delayable(struct k_work_delayable *w, void (*handler)(struct k_work *)) {
    w->work.handler = handler;
}
static int k_work_reschedule(struct k_work_delayable *w, int delay) { return 0; }
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

SCENARIOS = r"""

static int notify(int type) {
#if TEST_IDLE || TEST_USB || TEST_SUSPEND
    zmk_event_t event = {.type = type};
    return rgb_underglow_event_listener(&event);
#else
    return -ENOTSUP;
#endif
}
static int read_settings(void *data, void *destination, size_t length) {
    memcpy(destination, data, length);
    return length;
}
int main(void) {
#if TEST_IDLE || TEST_USB || TEST_SUSPEND
    assert(notify(2) == -ENODEV); // Early USB events must not poison remembered state.
#endif
    assert(zmk_rgb_underglow_init() == 0);
    assert(state.on == (!TEST_SUSPEND || !TEST_BOOT_SUSPENDED));
    if (TEST_SUSPEND && TEST_BOOT_SUSPENDED)
        assert(!underglow_tick.active);
#if TEST_IDLE || TEST_USB || TEST_SUSPEND
    // Check every combination of enabled gates, including conflicting wake inputs.
    for (int active = 0; active < 2; active++)
        for (int power = 0; power < 2; power++)
            for (int suspended = 0; suspended < 2; suspended++) {
                activity_active = active;
                usb_powered = power;
                usb_status = suspended ? USB_DC_SUSPEND : USB_DC_CONFIGURED;
                bool expected =
                    (!TEST_IDLE || active) && (!TEST_USB || power) && (!TEST_SUSPEND || !suspended);
                assert(rgb_underglow_should_be_awake() == expected);
            }
#endif
    activity_active = true;
    usb_powered = true;
    usb_status = USB_DC_RESUME;
    notify(2);
    assert(state.on); // Resume also restores a keyboard initialized during suspend.
#if TEST_SUSPEND && CONFIG_SETTINGS
    // Restore saved preferences with both awake and already-suspended hosts.
    for(int suspended=0;suspended<2;suspended++)for(int preference=0;preference<2;preference++) {
        usb_status=suspended?USB_DC_SUSPEND:USB_DC_CONFIGURED;
        struct rgb_underglow_state saved=state;
        saved.on=preference;
        assert(rgb_settings_set("state",sizeof(saved),read_settings,&saved)==0);
        assert(state.on==(preference && !suspended));
        if(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)assert(powered==(preference && !suspended));
        usb_status=USB_DC_RESUME;notify(2);
        assert(state.on==preference);
    }
#endif
    state.color = (struct zmk_led_hsb){123, 45, 67};
    state.current_effect = UNDERGLOW_EFFECT_SWIRL;
    zmk_rgb_underglow_on();
    usb_status = USB_DC_SUSPEND;
    notify(2);
    assert(state.on == !TEST_SUSPEND);
    int before_duplicate = writes;
    notify(2);
    assert(writes == before_duplicate);
    if (TEST_SUSPEND && CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
        assert(!powered);

    // USB resume must not override an enabled idle condition.
    activity_active = false;
    usb_status = USB_DC_RESUME;
    notify(2);
    assert(state.on == (!(TEST_IDLE && (TEST_USB || TEST_SUSPEND))));
    activity_active = true;
    notify(1);
    assert(state.on);

    // A keyboard activity event must not override USB suspend.
    usb_status = USB_DC_SUSPEND;
    notify(2);
    notify(1);
    assert(state.on == !TEST_SUSPEND);

    // Disconnection permits Bluetooth lighting unless USB-power gating is enabled.
    usb_powered = false;
    usb_status = USB_DC_DISCONNECTED;
    notify(2);
    assert(state.on == !TEST_USB);
    usb_powered = true;
    usb_status = USB_DC_CONFIGURED;
    notify(2);
    assert(state.on);

    // The user's already-off preference survives a suspend/resume cycle.
    zmk_rgb_underglow_off();
    usb_status = USB_DC_SUSPEND;
    notify(2);
    notify(2);
    usb_status = USB_DC_RESUME;
    notify(2);
    assert(!state.on);
    assert(state.color.h == 123 && state.color.s == 45 && state.color.b == 67);
    assert(state.current_effect == UNDERGLOW_EFFECT_SWIRL);
    return 0;
}
"""

with tempfile.TemporaryDirectory(prefix="zmk-usb-suspend-test-") as directory:
    root = Path(directory)
    (root / "mocks.h").write_text(MOCKS)
    for header in re.findall(r"#include <([^>]+)>", source.read_text()):
        if header in ["math.h", "stdlib.h", "string.h"]:
            continue
        path = root / header
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "mocks.h"\n')
    harness = root / "test.c"
    harness.write_text('#include "' + str(source) + '"\n' + SCENARIOS)
    compiler_flags = (
        ["-Wno-gnu-designator"]
        if "clang" in subprocess.check_output(["cc", "--version"], text=True).lower()
        else []
    )
    for idle, usb, suspend, boot_suspended, power, settings in product(
        [0, 1], repeat=6
    ):
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
                f"-DTEST_IDLE={idle}",
                f"-DTEST_USB={usb}",
                f"-DTEST_SUSPEND={suspend}",
                f"-DTEST_BOOT_SUSPENDED={boot_suspended}",
                f"-DTEST_SETTINGS={settings}",
                f"-DCONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER={power}",
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
    "PASS: 64 full-source C configurations; suspend/resume, boot during suspend, duplicate events, settings restoration, combined gates, USB disconnection, power control, preserved settings, and already-off state"
)
