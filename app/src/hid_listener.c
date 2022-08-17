/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <zmk/endpoints.h>

static int hid_listener_keycode_pressed(const struct zmk_keycode_state_changed *ev) {
    int err, explicit_mods_changed, implicit_mods_changed;

    LOG_DBG("usage_page 0x%02X keycode 0x%02X implicit_mods 0x%02X explicit_mods 0x%02X",
            ev->usage_page, ev->keycode, ev->implicit_modifiers, ev->explicit_modifiers);
    err = zmk_hid_press(ZMK_HID_USAGE(ev->usage_page, ev->keycode));
    if (err < 0) {
        LOG_DBG("Unable to press keycode");
        return err;
    }
    explicit_mods_changed = zmk_hid_register_mods(ev->explicit_modifiers);
    implicit_mods_changed = zmk_hid_implicit_modifiers_press(ev->implicit_modifiers);
    if (ev->usage_page != HID_USAGE_KEY &&
        (explicit_mods_changed > 0 || implicit_mods_changed > 0)) {
        err = zmk_endpoints_send_report(HID_USAGE_KEY);
        if (err < 0) {
            LOG_ERR("Failed to send key report for changed mofifiers for consumer page event (%d)",
                    err);
        }
    }

    return zmk_endpoints_send_report(ev->usage_page);
}

static int hid_listener_keycode_released(const struct zmk_keycode_state_changed *ev) {
    int err, explicit_mods_changed, implicit_mods_changed;

    LOG_DBG("usage_page 0x%02X keycode 0x%02X implicit_mods 0x%02X explicit_mods 0x%02X",
            ev->usage_page, ev->keycode, ev->implicit_modifiers, ev->explicit_modifiers);
    err = zmk_hid_release(ZMK_HID_USAGE(ev->usage_page, ev->keycode));
    if (err < 0) {
        LOG_DBG("Unable to release keycode");
        return err;
    }

    explicit_mods_changed = zmk_hid_unregister_mods(ev->explicit_modifiers);
    // There is a minor issue with this code.
    // If LC(A) is pressed, then LS(B), then LC(A) is released, the shift for B will be released
    // prematurely. This causes if LS(B) to repeat like Bbbbbbbb when pressed for a long time.
    // Solving this would require keeping track of which key's implicit modifiers are currently
    // active and only releasing modifiers at that time.
    implicit_mods_changed = zmk_hid_implicit_modifiers_release();
    ;
    if (ev->usage_page != HID_USAGE_KEY &&
        (explicit_mods_changed > 0 || implicit_mods_changed > 0)) {
        err = zmk_endpoints_send_report(HID_USAGE_KEY);
        if (err < 0) {
            LOG_ERR("Failed to send key report for changed mofifiers for consumer page event (%d)",
                    err);
        }
    }
    return zmk_endpoints_send_report(ev->usage_page);
}

int hid_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev) {
        if (ev->state) {
            hid_listener_keycode_pressed(ev);
        } else {
            hid_listener_keycode_released(ev);
        }
    }
    return 0;
}

ZMK_LISTENER(hid_listener, hid_listener);
ZMK_SUBSCRIPTION(hid_listener, zmk_keycode_state_changed);
