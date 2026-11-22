/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/peripheral.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
#include <zmk/hid_indicators.h>
#include <zmk/events/hid_indicators_changed.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

static struct {
    bool known;
    zmk_hid_indicators_t indicators;
} hid_indicators;

zmk_hid_indicators_t zmk_hid_indicators_get_current_profile(void) {
    return hid_indicators.indicators;
}

static void handle_hid_indicators(const struct zmk_split_transport_central_command *cmd) {
    if (hid_indicators.known &&
        hid_indicators.indicators == cmd->data.set_hid_indicators.indicators) {
        return;
    }

    hid_indicators.known = true;
    hid_indicators.indicators = cmd->data.set_hid_indicators.indicators;
    LOG_DBG("Raising HID indicators changed event: %x", hid_indicators.indicators);

    raise_zmk_hid_indicators_changed(
        (struct zmk_hid_indicators_changed){.indicators = hid_indicators.indicators});
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

int zmk_split_peripheral_state_sync_handle(const struct zmk_split_transport_central_command *cmd) {
    switch (cmd->type) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_HID_INDICATORS:
        handle_hid_indicators(cmd);
        break;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    default:
        return -ENOTSUP;
    }

    return 0;
}

// While the central is unreachable, every domain returns to its boot state. A zeroed payload equals
// the boot value, so only values that change raise events, and the next value from the central
// raises again, as after boot.
static void reset_work_cb(struct k_work *work) {
    static const struct zmk_split_transport_central_command boot_state;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    if (hid_indicators.known) {
        handle_hid_indicators(&boot_state);
        hid_indicators.known = false;
    }
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
}

static K_WORK_DEFINE(reset_work, reset_work_cb);

// Only BLE peripherals raise this event; wired ones keep the last received state.
static int state_sync_status_listener(const zmk_event_t *eh) {
    const struct zmk_split_peripheral_status_changed *ev =
        as_zmk_split_peripheral_status_changed(eh);

    // The system work queue also handles received commands, so the reset cannot interleave with
    // them.
    if (ev && !ev->connected) {
        k_work_submit(&reset_work);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(split_peripheral_state_sync, state_sync_status_listener);
ZMK_SUBSCRIPTION(split_peripheral_state_sync, zmk_split_peripheral_status_changed);
