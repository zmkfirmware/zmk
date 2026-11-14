/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "internal.h"

static void led_matrix_start_status_overlay(uint8_t sources, uint32_t timeout_ms,
                                            bool merge_sources) {
    uint32_t now_ms = k_uptime_get_32();
    uint32_t deadline_ms = now_ms + timeout_ms;

    if (merge_sources && status_mode == STATUS_MODE_TRANSIENT &&
        (int32_t)(transient_deadline_ms - deadline_ms) > 0) {
        deadline_ms = transient_deadline_ms;
    }

    transient_deadline_ms = deadline_ms;
    status_mode = STATUS_MODE_TRANSIENT;
    transient_sources = merge_sources ? (transient_sources | sources) : sources;

    uint32_t effective_timeout_ms = MAX((uint32_t)1U, deadline_ms - now_ms);
    (void)led_matrix_reschedule_delayable_work(&led_matrix_clear_status_work,
                                               K_MSEC(effective_timeout_ms));

    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_STATUS);
}

static bool led_matrix_show_auto_status(uint8_t sources, uint32_t timeout_ms,
                                        enum led_matrix_status_wake_context wake_context,
                                        bool merge_sources) {
    if (sources == 0U || timeout_ms == 0U || led_matrix_is_sleep_suspended()) {
        return false;
    }

    if (!led_matrix_auto_status_wake_allowed()) {
        return false;
    }

    if (led_matrix_wake_context_requires_transient_status(wake_context)) {
        if (!led_matrix_activate_status_wake_context(wake_context)) {
            return false;
        }
    } else if (!led_matrix_render_enabled()) {
        return false;
    }

    led_matrix_start_status_overlay(sources, timeout_ms, merge_sources);
    led_matrix_schedule_render();
    return true;
}

static void led_matrix_refresh_visible_status_sources(uint8_t sources) {
    if (sources == 0U || !led_matrix_render_enabled() ||
        (current_status_sources() & sources) == 0U) {
        return;
    }

    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_STATUS);
    led_matrix_schedule_render();
}

static void led_matrix_show_auto_status_sources(uint8_t sources, bool merge_sources) {
    enum led_matrix_status_wake_context wake_context = led_matrix_resolve_status_wake_context();

    if (!led_matrix_auto_status_wake_allowed()) {
        return;
    }

    led_matrix_refresh_automatic_persistent_status_sources();

    if (led_matrix_wake_context_requires_transient_status(wake_context)) {
        uint8_t transient_sources = led_matrix_filter_available_status_sources(sources);

        if (transient_sources != 0U) {
            (void)led_matrix_show_auto_status(
                transient_sources,
                timeout_for_sources_with_wake_context(transient_sources, wake_context),
                wake_context, merge_sources);
        }

        return;
    }

    uint8_t persistent_sources = sources & automatic_persistent_status_sources_mask;
    uint8_t transient_only_sources = led_matrix_filter_status_sources(sources, false);

    led_matrix_refresh_visible_status_sources(persistent_sources);

    if (transient_only_sources != 0U) {
        (void)led_matrix_show_auto_status(
            transient_only_sources,
            timeout_for_sources_with_wake_context(transient_only_sources, wake_context),
            wake_context, merge_sources);
    }
}

static void led_matrix_refresh_layer_snapshot(void) {
#if LED_MATRIX_HAS_CENTRAL_STATE
    zmk_keymap_layer_index_t layer_idx = zmk_keymap_highest_layer_active();
    snapshot.active_layer = zmk_keymap_layer_index_to_id(layer_idx);
#else
    snapshot.active_layer = 0;
#endif
}

static bool same_status(enum led_matrix_status_source src, struct led_matrix_snapshot *updated) {
    switch (src) {
    case LED_MATRIX_STATUS_BATTERY:
        return updated->battery_soc == snapshot.battery_soc;
    case LED_MATRIX_STATUS_USB:
        return updated->usb_state == snapshot.usb_state;
    case LED_MATRIX_STATUS_BLE:
        return updated->ble_profile == snapshot.ble_profile &&
               updated->ble_connected == snapshot.ble_connected &&
               updated->ble_open == snapshot.ble_open;
    case LED_MATRIX_STATUS_HID:
        return updated->hid == snapshot.hid;
    default:
        return true;
    }
}

static void trigger_ripple_for_position(uint32_t position) {
    const struct led_matrix_profile_cfg *profile = selected_profile_cfg();

    if (position >= ZMK_KEYMAP_LEN || grid_cell_count == 0U || profile == NULL ||
        profile->renderer != LED_MATRIX_RENDERER_RIPPLE) {
        return;
    }

    uint16_t led = position_to_led[position];
    if (led == LM_NOLED || led >= LOGICAL_LED_COUNT) {
        return;
    }

    uint16_t cell = position_to_cell[position];
    if (cell == LM_NOLED || cell >= grid_cell_count) {
        return;
    }

    ripple_state.active = true;
    ripple_state.origin_cell = cell;
    ripple_state.start_time_ms = k_uptime_get_32();

    if (led_matrix_render_enabled()) {
        led_matrix_schedule_render();
    }
}

static void led_matrix_handle_output_transport_change(enum zmk_transport previous_transport,
                                                      enum zmk_transport new_transport) {
    if (previous_transport == new_transport) {
        return;
    }

    uint8_t sources = 0U;
    switch (new_transport) {
    case ZMK_TRANSPORT_BLE:
        sources = LED_MATRIX_STATUS_USB | LED_MATRIX_STATUS_BLE;
        break;
    case ZMK_TRANSPORT_USB:
        sources = LED_MATRIX_STATUS_USB;
        break;
    default:
        return;
    }

    LOG_DBG("LED matrix output transport changed: %d -> %d", previous_transport, new_transport);
    led_matrix_show_auto_status_sources(sources, true);
}

static void
led_matrix_handle_battery_status_event(const struct zmk_battery_state_changed *battery_ev) {
    struct led_matrix_snapshot updated = snapshot;
    updated.battery_soc = battery_ev != NULL ? MIN(battery_ev->state_of_charge, (uint8_t)100)
                                             : led_matrix_current_battery_soc();

    if (same_status(LED_MATRIX_STATUS_BATTERY, &updated)) {
        return;
    }

    snapshot = updated;

    if (!led_matrix_render_enabled() && !led_matrix_base_render_enabled()) {
        LOG_DBG("LED matrix battery state updated snapshot only (OFF)");
        return;
    }

    LOG_DBG("LED matrix battery state changed: %u%%", updated.battery_soc);
    if (!led_matrix_auto_status_wake_allowed()) {
        LOG_DBG("LED matrix battery state change updated snapshot only");
        return;
    }

    led_matrix_refresh_automatic_persistent_status_sources();

    if ((current_status_sources() & LED_MATRIX_STATUS_BATTERY) != 0U) {
        led_matrix_refresh_visible_status_sources(LED_MATRIX_STATUS_BATTERY);
        LOG_DBG("LED matrix refreshed visible battery status after battery state change");
    } else {
        LOG_DBG("LED matrix battery state change updated snapshot only");
    }
}

static void led_matrix_handle_usb_status_event(const struct zmk_usb_conn_state_changed *usb_ev) {
    struct led_matrix_snapshot previous = snapshot;
    struct led_matrix_snapshot updated = snapshot;

    updated.battery_soc = led_matrix_current_battery_soc();
    updated.usb_state = usb_ev != NULL ? usb_ev->conn_state : led_matrix_current_usb_state();
    updated.output_transport = led_matrix_current_output_transport();

    if (same_status(LED_MATRIX_STATUS_USB, &updated) &&
        previous.output_transport == updated.output_transport) {
        return;
    }

    snapshot = updated;
    /* Activity may already be IDLE when VBUS disappears: there will be no second idle event. */
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
    if ((previous.usb_state == ZMK_USB_CONN_NONE) != (updated.usb_state == ZMK_USB_CONN_NONE)) {
        if (led_matrix_usb_powered()) {
            (void)led_matrix_auto_resume_if_needed();
            led_matrix_cancel_idle_auto_off();
        } else if (zmk_activity_get_state() == ZMK_ACTIVITY_IDLE) {
            (void)led_matrix_auto_suspend();
        }
    }
#endif
    if (updated.usb_state != previous.usb_state) {
        uint8_t sources = LED_MATRIX_STATUS_USB;

        if ((previous.usb_state == ZMK_USB_CONN_NONE) != (updated.usb_state == ZMK_USB_CONN_NONE)) {
            sources |= LED_MATRIX_STATUS_BATTERY;
            LOG_DBG("LED matrix USB boundary change includes battery status");
        }

        LOG_DBG("LED matrix USB state changed: %d -> %d", previous.usb_state, updated.usb_state);
        led_matrix_show_auto_status_sources(sources, true);
    }

    led_matrix_handle_output_transport_change(previous.output_transport, updated.output_transport);
}

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS) && LED_MATRIX_HAS_CENTRAL_STATE
static void led_matrix_handle_hid_status_event(void) {
    struct led_matrix_snapshot updated = snapshot;
    updated.hid = led_matrix_current_hid_indicators();

    if (same_status(LED_MATRIX_STATUS_HID, &updated)) {
        return;
    }

    snapshot = updated;
    led_matrix_show_auto_status_sources(LED_MATRIX_STATUS_HID, true);
}
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE) && LED_MATRIX_HAS_CENTRAL_STATE
static uint32_t led_matrix_ble_pairing_timeout_ms(void) {
    return (uint32_t)BLE_TIMEOUT_MS * BLE_PAIRING_TIMEOUT_MULTIPLIER;
}

static bool led_matrix_start_ble_pairing_transient(enum led_matrix_status_wake_context wake_context,
                                                   bool merge_sources) {
    uint32_t timeout_ms = led_matrix_ble_pairing_timeout_ms();

    ble_pairing_transient_active = false;
    ble_pairing_transient_allows_completion_overlay = false;

    if (timeout_ms == 0U) {
        return false;
    }

    if (led_matrix_show_auto_status(LED_MATRIX_STATUS_BLE, timeout_ms, wake_context,
                                    merge_sources)) {
        ble_pairing_transient_active = true;
        ble_pairing_transient_allows_completion_overlay = true;
        return true;
    }

    return false;
}

static void led_matrix_handle_ble_status_event(void) {
    struct led_matrix_snapshot previous = snapshot;
    struct led_matrix_snapshot updated = snapshot;

    updated.ble_profile = led_matrix_current_ble_profile();
    updated.ble_connected = led_matrix_current_ble_connected();
    updated.ble_open = led_matrix_current_ble_open();
    updated.output_transport = led_matrix_current_output_transport();

    bool ble_changed = !same_status(LED_MATRIX_STATUS_BLE, &updated);
    bool output_changed = previous.output_transport != updated.output_transport;

    if (!ble_changed && !output_changed) {
        return;
    }

    snapshot = updated;

    if (ble_changed) {
        bool switched_profile = previous.ble_profile != updated.ble_profile;
        bool was_pairing_transient = ble_pairing_transient_active;
        bool completion_overlay_allowed = ble_pairing_transient_allows_completion_overlay;
        bool pairing_transient_now = BLE_PAIRING_HOLD && updated.ble_open;
        bool pairing_session_settled = BLE_PAIRING_HOLD && previous.ble_open && !updated.ble_open;
        enum led_matrix_status_wake_context wake_context = led_matrix_resolve_status_wake_context();

        if (pairing_transient_now) {
            if (led_matrix_start_ble_pairing_transient(wake_context, true)) {
                if (!was_pairing_transient) {
                    LOG_DBG("LED matrix entered BLE pairing transient for profile %d",
                            updated.ble_profile);
                } else if (switched_profile) {
                    LOG_DBG("LED matrix BLE profile switch during pairing transient: %d -> %d",
                            previous.ble_profile, updated.ble_profile);
                }
            }
        } else if (pairing_session_settled) {
            ble_pairing_transient_active = false;
            ble_pairing_transient_allows_completion_overlay = false;

            if (completion_overlay_allowed) {
                LOG_DBG("LED matrix completed BLE pairing, showing settled BLE status%s",
                        switched_profile ? " after profile switch" : "");
                led_matrix_show_auto_status_sources(LED_MATRIX_STATUS_BLE, true);
            } else {
                LOG_DBG("LED matrix BLE pairing settled after transient expiry%s",
                        switched_profile ? " after profile switch" : "");
            }
        } else {
            ble_pairing_transient_active = false;
            ble_pairing_transient_allows_completion_overlay = false;

            if (switched_profile) {
                LOG_DBG("LED matrix BLE profile switch: %d -> %d", previous.ble_profile,
                        updated.ble_profile);
            }

            led_matrix_show_auto_status_sources(LED_MATRIX_STATUS_BLE, true);
        }

        if (was_pairing_transient != ble_pairing_transient_active && !led_matrix_render_enabled()) {
            (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_STATUS);
        }
    }

    led_matrix_handle_output_transport_change(previous.output_transport, updated.output_transport);
}
#endif

#if LED_MATRIX_HAS_CENTRAL_STATE
static void led_matrix_handle_endpoint_status_event(void) {
    struct led_matrix_snapshot updated = snapshot;
    updated.output_transport = led_matrix_current_output_transport();

    if (updated.output_transport == snapshot.output_transport) {
        return;
    }

    enum zmk_transport previous_transport = snapshot.output_transport;
    snapshot = updated;
    led_matrix_handle_output_transport_change(previous_transport, updated.output_transport);
}
#endif

static void led_matrix_handle_position_event(uint32_t position, bool pressed) {
    if (pressed) {
        (void)led_matrix_auto_resume_if_needed();
    }

    const struct led_matrix_profile_cfg *profile = selected_profile_cfg();
    bool keypress_profile = profile != NULL && profile->renderer == LED_MATRIX_RENDERER_KEYPRESS;
    bool rendering = led_matrix_render_enabled();

    /* Track releases even while off, and retain the first key that wakes idle output. */
    if (position < ZMK_KEYMAP_LEN) {
        pressed_positions[position] = pressed;
        keypress_release_fade_start_ms[position] = 0U;
        keypress_release_deadline_ms[position] = 0U;
        if (!pressed && rendering && keypress_profile &&
            (profile->keypress_afterglow_ms > 0 || profile->keypress_fadeout_ms > 0)) {
            uint32_t fade_start =
                k_uptime_get_32() + (uint32_t)MAX(profile->keypress_afterglow_ms, 0);
            uint32_t deadline = fade_start + (uint32_t)MAX(profile->keypress_fadeout_ms, 0);
            keypress_release_fade_start_ms[position] = fade_start;
            /* Zero is the inactive sentinel; crossing uptime zero must still expire. */
            keypress_release_deadline_ms[position] = deadline == 0U ? 1U : deadline;
        }
    }

    if (!rendering) {
        return;
    }
    if (keypress_profile) {
        led_matrix_schedule_render();
    }
    if (pressed) {
        trigger_ripple_for_position(position);
    }
}

static int led_matrix_listener(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *activity_ev = as_zmk_activity_state_changed(eh);
    if (activity_ev != NULL) {
        switch (activity_ev->state) {
        case ZMK_ACTIVITY_ACTIVE:
            led_matrix_clear_idle_started();
            if (led_matrix_is_sleep_suspended()) {
                sleep_suspended = false;
                if (!led_matrix_auto_resume_if_needed()) {
                    (void)led_matrix_resume_runtime_output(ZMK_LED_MATRIX_STATE_CHANGE_ENABLED |
                                                           ZMK_LED_MATRIX_STATE_CHANGE_STATUS |
                                                           ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR);
                }
            }
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
            (void)led_matrix_auto_resume_if_needed();
            led_matrix_schedule_idle_auto_off_if_needed();
#endif
            break;
        case ZMK_ACTIVITY_IDLE:
            if (!led_matrix_is_sleep_suspended()) {
                led_matrix_note_idle_started();
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
                (void)led_matrix_auto_suspend();
#endif
            }
            break;
        case ZMK_ACTIVITY_SLEEP:
            if (!led_matrix_is_sleep_suspended()) {
                sleep_suspended = true;
                led_matrix_cancel_idle_auto_off();
                LOG_DBG("LED matrix sleep suspend -> %s",
                        led_matrix_runtime_mode_name(led_matrix_current_runtime_mode()));
                (void)led_matrix_suspend_runtime_output_sync(
                    ZMK_LED_MATRIX_STATE_CHANGE_ENABLED | ZMK_LED_MATRIX_STATE_CHANGE_STATUS |
                    ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR);
            }
            break;
        default:
            break;
        }

        return 0;
    }

    const struct zmk_battery_state_changed *battery_ev = as_zmk_battery_state_changed(eh);
    if (battery_ev != NULL) {
        led_matrix_handle_battery_status_event(battery_ev);
        return 0;
    }

    const struct zmk_usb_conn_state_changed *usb_ev = as_zmk_usb_conn_state_changed(eh);
    if (usb_ev != NULL) {
        led_matrix_handle_usb_status_event(usb_ev);
        return 0;
    }

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS) && LED_MATRIX_HAS_CENTRAL_STATE
    if (as_zmk_hid_indicators_changed(eh) != NULL) {
        led_matrix_handle_hid_status_event();
        return 0;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE) && LED_MATRIX_HAS_CENTRAL_STATE
    if (as_zmk_ble_active_profile_changed(eh) != NULL) {
        led_matrix_handle_ble_status_event();
        return 0;
    }
#endif

#if LED_MATRIX_HAS_CENTRAL_STATE
    if (as_zmk_endpoint_changed(eh) != NULL) {
        led_matrix_handle_endpoint_status_event();
        return 0;
    }
#endif

    struct zmk_position_state_changed *position_ev = as_zmk_position_state_changed(eh);
    if (position_ev != NULL) {
        led_matrix_handle_position_event(position_ev->position, position_ev->state);
        return 0;
    }

#if LED_MATRIX_HAS_CENTRAL_STATE
    if (as_zmk_layer_state_changed(eh) != NULL) {
        zmk_keymap_layer_id_t old_layer = snapshot.active_layer;
        led_matrix_refresh_layer_snapshot();

        if (snapshot.active_layer == old_layer) {
            return 0;
        }

        if (!led_matrix_base_render_enabled()) {
            LOG_DBG("LED matrix layer changed: %d -> %d (OFF, snapshot only)", old_layer,
                    snapshot.active_layer);
            return 0;
        }

        LOG_DBG("LED matrix active layer changed: %d -> %d", old_layer, snapshot.active_layer);
        led_matrix_refresh_layer_color(true);
        led_matrix_schedule_render();

        return 0;
    }
#endif

    return -ENOTSUP;
}

ZMK_LISTENER(led_matrix, led_matrix_listener);
ZMK_SUBSCRIPTION(led_matrix, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(led_matrix, zmk_usb_conn_state_changed);
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS) && LED_MATRIX_HAS_CENTRAL_STATE
ZMK_SUBSCRIPTION(led_matrix, zmk_hid_indicators_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE) && LED_MATRIX_HAS_CENTRAL_STATE
ZMK_SUBSCRIPTION(led_matrix, zmk_ble_active_profile_changed);
#endif
#if LED_MATRIX_HAS_CENTRAL_STATE
ZMK_SUBSCRIPTION(led_matrix, zmk_endpoint_changed);
#endif
ZMK_SUBSCRIPTION(led_matrix, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(led_matrix, zmk_position_state_changed);
#if LED_MATRIX_HAS_CENTRAL_STATE
ZMK_SUBSCRIPTION(led_matrix, zmk_layer_state_changed);
#endif
