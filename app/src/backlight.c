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
#endif // IS_ENABLED(CONFIG_SETTINGS)

static int zmk_backlight_save_state() {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_delayed_work_cancel(&backlight_save_work);
    return k_delayed_work_submit(&backlight_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#else
    return 0;
#endif
}

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

int zmk_backlight_set_on(bool on) {
    if (!state.on && state.brightness == 0) {
        state.brightness = CONFIG_ZMK_BACKLIGHT_BRT_STEP;
    }
    state.on = on;

    int rc = zmk_backlight_update();
    if (rc != 0) {
        return rc;
    }

    return zmk_backlight_save_state();
}

bool zmk_backlight_is_on() { return state.on; }

int zmk_backlight_set_brt(int brt) {
    state.on = (brt > 0);
    state.brightness = CLAMP(brt, 0, 100);

    int rc = zmk_backlight_update();
    if (rc != 0) {
        return rc;
    }

    return zmk_backlight_save_state();
}

int zmk_backlight_get_brt() { return state.on ? state.brightness : 0; }

int zmk_backlight_toggle() { return zmk_backlight_set_on(!state.on); }

int zmk_backlight_on() { return zmk_backlight_set_on(true); }

int zmk_backlight_off() { return zmk_backlight_set_on(false); }

int zmk_backlight_inc() {
    if (!state.on) {
        return zmk_backlight_set_brt(MAX(state.brightness, CONFIG_ZMK_BACKLIGHT_BRT_STEP));
    }
    return zmk_backlight_set_brt(state.brightness + CONFIG_ZMK_BACKLIGHT_BRT_STEP);
}

int zmk_backlight_dec() {
    return zmk_backlight_set_brt(state.brightness - CONFIG_ZMK_BACKLIGHT_BRT_STEP);
}

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE)
static bool auto_off_idle_prev_state = false;
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
static bool auto_off_usb_prev_state = false;
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE) || IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
static int backlight_event_listener(const zmk_event_t *eh) {

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE)
    if (as_zmk_activity_state_changed(eh)) {
        bool new_state = (zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE);
        if (state.on == new_state) {
            return 0;
        }
        if (new_state) {
            state.on = auto_off_idle_prev_state;
            auto_off_idle_prev_state = false;
        } else {
            state.on = false;
            auto_off_idle_prev_state = true;
        }
        return zmk_backlight_update();
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
    if (as_zmk_usb_conn_state_changed(eh)) {
        bool new_state = zmk_usb_is_powered();
        if (state.on == new_state) {
            return 0;
        }
        if (new_state) {
            state.on = auto_off_usb_prev_state;
            auto_off_usb_prev_state = false;
        } else {
            state.on = false;
            auto_off_usb_prev_state = true;
        }
        return zmk_backlight_update();
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
