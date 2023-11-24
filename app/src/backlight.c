/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <zmk/activity.h>
#include <zmk/backlight.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/workqueue.h>

// Pull data sending framework if central, data receiving eventt if peripheral(s)
#if ZMK_BLE_IS_CENTRAL
#include <zmk/split/bluetooth/central.h>
#elif IS_ENABLED(CONFIG_ZMK_SPLIT)
#include <zmk/events/split_data_xfer_event.h>
#endif

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

#if ZMK_BLE_IS_CENTRAL

static void zmk_backlight_send_state(struct k_work *work) {
    LOG_HEXDUMP_DBG(&state, sizeof(struct backlight_state), "backlight state");
    int err = zmk_split_central_send_data(DATA_TAG_BACKLIGHT_STATE, sizeof(struct backlight_state),
                                          (uint8_t *)&state);
    if (err) {
        LOG_ERR("send failed (err %d)", err);
    }
}

K_WORK_DEFINE(backlight_send_state_work, zmk_backlight_send_state);

#endif

static int zmk_backlight_update(void) {
#if ZMK_BLE_IS_CENTRAL
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &backlight_send_state_work);
#endif
    uint8_t brt = zmk_backlight_get_brt();
    LOG_DBG("Update backlight brightness: %d%%", brt);

    for (int i = 0; i < BACKLIGHT_NUM_LEDS; i++) {
        int rc = led_set_brightness(backlight_dev, i, brt);
        if (rc != 0) {
            LOG_ERR("Failed to update backlight LED %d: %d", i, rc);
            return rc;
        }
    }
    return 0;
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int backlight_settings_load_cb(const char *name, size_t len, settings_read_cb read_cb,
                                      void *cb_arg, void *param) {
    const char *next;
    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(state)) {
            return -EINVAL;
        }

        int rc = read_cb(cb_arg, &state, sizeof(state));
        return MIN(rc, 0);
    }
    return -ENOENT;
}

static void backlight_save_work_handler(struct k_work *work) {
    settings_save_one("backlight/state", &state, sizeof(state));
}

static struct k_work_delayable backlight_save_work;
#endif

static int zmk_backlight_init(void) {
    if (!device_is_ready(backlight_dev)) {
        LOG_ERR("Backlight device \"%s\" is not ready", backlight_dev->name);
        return -ENODEV;
    }

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();
    int rc = settings_load_subtree_direct("backlight", backlight_settings_load_cb, NULL);
    if (rc != 0) {
        LOG_ERR("Failed to load backlight settings: %d", rc);
    }
    k_work_init_delayable(&backlight_save_work, backlight_save_work_handler);
#endif
#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
    state.on = zmk_usb_is_powered();
#endif
    return zmk_backlight_update();
}

static int zmk_backlight_update_and_save(void) {
    int rc = zmk_backlight_update();
    if (rc != 0) {
        return rc;
    }

#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = k_work_reschedule(&backlight_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
    return MIN(ret, 0);
#else
    return 0;
#endif
}

int zmk_backlight_on(void) {
    state.brightness = MAX(state.brightness, CONFIG_ZMK_BACKLIGHT_BRT_STEP);
    state.on = true;
    return zmk_backlight_update_and_save();
}

int zmk_backlight_off(void) {
    state.on = false;
    return zmk_backlight_update_and_save();
}

int zmk_backlight_toggle(void) { return state.on ? zmk_backlight_off() : zmk_backlight_on(); }

bool zmk_backlight_is_on(void) { return state.on; }

int zmk_backlight_set_brt(uint8_t brightness) {
    state.brightness = MIN(brightness, BRT_MAX);
    state.on = (state.brightness > 0);
    return zmk_backlight_update_and_save();
}

uint8_t zmk_backlight_get_brt(void) { return state.on ? state.brightness : 0; }

uint8_t zmk_backlight_calc_brt(int direction) {
    int brt = state.brightness + (direction * CONFIG_ZMK_BACKLIGHT_BRT_STEP);
    return CLAMP(brt, 0, BRT_MAX);
}

uint8_t zmk_backlight_calc_brt_cycle(void) {
    if (state.brightness == BRT_MAX) {
        return 0;
    } else {
        return zmk_backlight_calc_brt(1);
    }
}

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE) || IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
static int backlight_auto_state(bool *prev_state, bool new_state) {
    if (state.on == new_state) {
        return 0;
    }
    state.on = new_state && *prev_state;
    *prev_state = !new_state;
    return zmk_backlight_update();
}
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE) ||                                              \
    IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB) || ZMK_BLE_IS_CENTRAL
static int backlight_event_listener(const zmk_event_t *eh) {

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE)
    if (as_zmk_activity_state_changed(eh)) {
        static bool prev_state = false;
        return backlight_auto_state(&prev_state, zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
    if (as_zmk_usb_conn_state_changed(eh)) {
        static bool prev_state = false;
        return backlight_auto_state(&prev_state, zmk_usb_is_powered());
    }
#endif

#if ZMK_BLE_IS_CENTRAL
    if (as_zmk_split_peripheral_status_changed(eh)) {
        // TODO: Have this only update when connected
        k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &backlight_send_state_work);
    }
#endif

    return -ENOTSUP;
}

ZMK_LISTENER(backlight, backlight_event_listener);
#endif // IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE) ||
       // IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB) || ZMK_BLE_IS_CENTRAL

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE)
ZMK_SUBSCRIPTION(backlight, zmk_activity_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB)
ZMK_SUBSCRIPTION(backlight, zmk_usb_conn_state_changed);
#endif

#if ZMK_BLE_IS_CENTRAL
ZMK_SUBSCRIPTION(backlight, zmk_split_peripheral_status_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

static int backlight_data_event_listener(const zmk_event_t *eh) {
    const struct zmk_split_data_xfer_event *ev = as_zmk_split_data_xfer_event(eh);
    if (ev->data_xfer.data_tag == DATA_TAG_BACKLIGHT_STATE) {
        LOG_DBG("Backlight Data received of size: %d", ev->data_xfer.data_size);
        LOG_HEXDUMP_DBG(ev->data_xfer.data, sizeof(struct zmk_split_data_xfer_event),
                        "received event:");
        memcpy(&state, ev->data_xfer.data, sizeof(struct backlight_state));
        LOG_HEXDUMP_DBG(&state, sizeof(struct backlight_state), "backlight state");
        zmk_backlight_update_and_save();
    }
    return 0;
}

ZMK_LISTENER(backlight_data, backlight_data_event_listener);
ZMK_SUBSCRIPTION(backlight_data, zmk_split_data_xfer_event);
#endif

SYS_INIT(zmk_backlight_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
