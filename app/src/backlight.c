/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <devicetree.h>
#include <init.h>
#include <kernel.h>

#include <drivers/led.h>
#include <logging/log.h>
#include <settings/settings.h>

#include <zmk/activity.h>
#include <zmk/backlight.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

BUILD_ASSERT(DT_HAS_CHOSEN(zmk_backlight),
             "CONFIG_ZMK_BACKLIGHT is enabled but no zmk,backlight chosen node found");

static const struct device *const backlight_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_backlight));

#define CHILD_COUNT(...) +1
#define DT_NUM_CHILD(node_id) (DT_FOREACH_CHILD(node_id, CHILD_COUNT))

#define BACKLIGHT_NUM_LEDS (DT_NUM_CHILD(DT_CHOSEN(zmk_backlight)))

#define BRT_MAX 100

struct backlight_state {
    uint8_t brightness;
    bool on;
};

static struct backlight_state state = {.brightness = CONFIG_ZMK_BACKLIGHT_BRT_START,
                                       .on = IS_ENABLED(CONFIG_ZMK_BACKLIGHT_ON_START)};

static int zmk_backlight_update() {
    uint8_t brt = state.on ? state.brightness : 0;
    for (int i = 0; i < BACKLIGHT_NUM_LEDS; i++) {
        int rc = led_set_brightness(backlight_dev, i, brt);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int backlight_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                  void *cb_arg) {
    const char *next;

    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(state)) {
            return -EINVAL;
        }

        int rc = read_cb(cb_arg, &state, sizeof(state));
        if (rc < 0) {
            return rc;
        }

        return zmk_backlight_update();
    }

    return -ENOENT;
}

static struct settings_handler backlight_conf = {.name = "backlight",
                                                 .h_set = backlight_settings_set};

static void zmk_backlight_save_state_work() {
    settings_save_one("backlight/state", &state, sizeof(state));
}

static struct k_delayed_work backlight_save_work;
#endif

static int zmk_backlight_init(const struct device *_arg) {
    if (!device_is_ready(backlight_dev)) {
        LOG_ERR("Backlight device \"%s\" is not ready", backlight_dev->name);
        return -ENODEV;
    }

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    int err = settings_register(&backlight_conf);
    if (err) {
        LOG_ERR("Failed to register the backlight settings handler (err %d)", err);
        return err;
    }

    k_delayed_work_init(&backlight_save_work, zmk_backlight_save_state_work);

    settings_load_subtree("backlight");
#endif

    return zmk_backlight_update();
}

static int zmk_backlight_save_state() {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_delayed_work_cancel(&backlight_save_work);
    return k_delayed_work_submit(&backlight_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#else
    return 0;
#endif
}

bool zmk_backlight_get_on() { return state.on; }

int zmk_backlight_on() {
    if (!state.on && state.brightness == 0) {
        state.brightness = CONFIG_ZMK_BACKLIGHT_BRT_STEP;
    }
    state.on = true;

    int rc = zmk_backlight_update();
    if (rc != 0) {
        return rc;
    }

    return zmk_backlight_save_state();
}

int zmk_backlight_off() {

    state.on = false;

    int rc = zmk_backlight_update();
    if (rc != 0) {
        return rc;
    }

    return zmk_backlight_save_state();
}

int zmk_backlight_get_brt() { return state.on ? state.brightness : 0; }

int zmk_backlight_toggle() { return state.on ? zmk_backlight_off() : zmk_backlight_on(); }

int zmk_backlight_set_brt(uint8_t brightness) {
    if (brightness > BRT_MAX) {
        brightness = BRT_MAX;
    }

    state.brightness = brightness;
    state.on = (brightness > 0);

    int rc = zmk_backlight_update();
    if (rc != 0) {
        return rc;
    }

    return zmk_backlight_save_state();
}

uint8_t zmk_backlight_calc_brt(int direction) {
    uint8_t brightness = state.brightness;

    int b = state.brightness + (direction * CONFIG_ZMK_BACKLIGHT_BRT_STEP);
    return CLAMP(b, 0, BRT_MAX);
}

int zmk_backlight_adjust_brt(int direction) {

    state.brightness = zmk_backlight_calc_brt(direction);
    state.on = (state.brightness > 0);

    int rc = zmk_backlight_update();
    if (rc != 0) {
        return rc;
    }

    return zmk_backlight_save_state();
}

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE)
static bool auto_off_idle_prev_state = false;
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
static bool auto_off_usb_prev_state = false;
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE) || IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
static int backlight_auto_state(bool *prev_state, bool *new_state) {
    if (state.on == *new_state) {
        return 0;
    }
    if (*new_state) {
        state.on = *prev_state;
        *prev_state = false;
        return zmk_backlight_on();
    } else {
        state.on = false;
        *prev_state = true;
        return zmk_backlight_off();
    }
}

static int backlight_event_listener(const zmk_event_t *eh) {

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE)
    if (as_zmk_activity_state_changed(eh)) {
        bool new_state = (zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE);
        return backlight_auto_state(&auto_off_idle_prev_state, &new_state);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
    if (as_zmk_usb_conn_state_changed(eh)) {
        bool new_state = zmk_usb_is_powered();
        return backlight_auto_state(&auto_off_usb_prev_state, &new_state);
    }
#endif

    return -ENOTSUP;
}

ZMK_LISTENER(backlight, backlight_event_listener);
#endif // IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE) ||
       // IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE)
ZMK_SUBSCRIPTION(backlight, zmk_activity_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
ZMK_SUBSCRIPTION(backlight, zmk_usb_conn_state_changed);
#endif

SYS_INIT(zmk_backlight_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
