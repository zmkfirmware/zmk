#!/usr/bin/env python3
"""Build native calculator tests in a temporary directory; no firmware SDK required."""
from pathlib import Path
import os
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
APP = HERE.parents[4]
SHIM = r'''
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define CONFIG_NUMPAD_CALCULATOR 1
#define IS_ENABLED(x) x
#define DT_INST(a,b) 0
#define DT_DRV_INST(n) 0
#define DT_PROP(n,p) PROP_##p
#define PROP_calculator_layer 3
#define PROP_function_layer 4
#define DT_HAS_COMPAT_STATUS_OKAY(x) 1
#define ZMK_BEHAVIOR_OPAQUE 0
#define ZMK_EV_EVENT_BUBBLE 0
#define ZMK_LISTENER(a,b)
#define ZMK_SUBSCRIPTION(a,b)
#define ZMK_EVENT_DECLARE(t) int raise_##t(struct t ev)
#define ZMK_EVENT_IMPL(t) int raise_##t(struct t ev) { return 0; }
#define K_MUTEX_DEFINE(n) static int n
#define K_FOREVER 0
#define k_mutex_lock(a,b) ((void)(a))
#define k_mutex_unlock(a) ((void)(a))
typedef int atomic_t;
static inline int atomic_get(atomic_t *a) { return *a; }
static inline void atomic_set(atomic_t *a,int b) { *a=b; }
static inline void atomic_clear(atomic_t *a) { *a=0; }
struct k_work { int dummy; };
#define K_WORK_DELAYABLE_DEFINE(n,f) struct k_work n
#define K_MSEC(x) x
#define k_work_reschedule(a,b) ((void)(a))
static inline int64_t k_uptime_get(void) { return 0; }
typedef struct { int dummy; } zmk_event_t;
struct zmk_layer_state_changed { uint8_t layer; bool state; };
static inline const struct zmk_layer_state_changed *as_zmk_layer_state_changed(const zmk_event_t *e) {
    return (const struct zmk_layer_state_changed *)e;
}
struct zmk_endpoint_instance { int transport; int profile; };
bool zmk_endpoint_is_connected(void);
struct zmk_endpoint_instance zmk_endpoint_get_selected(void);
bool zmk_endpoint_instance_eq(struct zmk_endpoint_instance a, struct zmk_endpoint_instance b);
bool zmk_hid_mod_is_pressed(int i);
bool zmk_hid_is_pressed(uint32_t code);
int raise_zmk_keycode_state_changed_from_encoded(uint32_t code, bool pressed, int64_t timestamp);
bool zmk_keymap_layer_active(uint8_t layer);
int zmk_keymap_layer_activate(uint8_t layer, bool locking);
int zmk_keymap_layer_deactivate(uint8_t layer, bool locking);
struct zmk_behavior_binding { uint32_t param1; };
struct zmk_behavior_binding_event { int position; };
struct behavior_driver_api {
    int (*binding_pressed)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
    int (*binding_released)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
};
#define BEHAVIOR_DT_INST_DEFINE(...)
'''

def run(*args):
    subprocess.run(args, check=True)

with tempfile.TemporaryDirectory(prefix="numpad-calculator-tests-") as temp:
    out = Path(temp)
    (out / "fake_zmk.h").write_text(SHIM)
    for name in ["zmk/event_manager.h", "zmk/endpoints.h", "zmk/hid.h", "zmk/keymap.h",
                 "zmk/behavior.h", "drivers/behavior.h", "zmk/events/endpoint_changed.h",
                 "zmk/events/keycode_state_changed.h", "zmk/events/layer_state_changed.h"]:
        header = out / name
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text('#include "fake_zmk.h"\n')
    flags = [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
             "-fsanitize=address,undefined", "-I" + str(HERE.parent)]
    run(*flags, str(HERE.parent / "engine.c"), str(HERE / "engine_test.c"), "-o", str(out / "engine"))
    run(str(out / "engine"))
    run(*flags, "-Wno-unused-parameter", "-Wno-unused-const-variable", "-I" + str(out),
        "-I" + str(APP / "include"), str(HERE.parent / "engine.c"),
        str(HERE / "runtime_test.c"), "-o", str(out / "runtime"))
    run(str(out / "runtime"))
    run(*flags, "-Wno-unused-parameter", "-Wno-unused-const-variable", "-I" + str(out),
        "-I" + str(APP / "include"), str(HERE / "disabled_test.c"), "-o", str(out / "disabled"))
    run(str(out / "disabled"))

from format_link_test import check_format_link

check_format_link(HERE)
