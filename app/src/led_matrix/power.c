/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "internal.h"

static bool runtime_first_frame_after_wake_pending;
static enum led_matrix_runtime_mode low_power_clear_notice_mode = LED_MATRIX_RUNTIME_MODE_ACTIVE;

#if LED_DRIVER_IS_STRIP && IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EXT_POWER)
static bool ext_power_state_known;
static bool ext_power_is_on;
#endif

#if LED_MATRIX_BACKEND_HAS_MANAGED_POWER
enum led_matrix_backend_power_mode {
    LED_MATRIX_BACKEND_POWER_MODE_SOFT = 0,
    LED_MATRIX_BACKEND_POWER_MODE_HARD_OFF,
};

#if LED_MATRIX_BACKEND_IS_COMPOSITE
static uint32_t backend_composite_capabilities;
#endif
static bool backend_power_state_known;
static bool backend_power_enabled;
static enum led_matrix_backend_power_mode backend_power_mode = LED_MATRIX_BACKEND_POWER_MODE_SOFT;
#endif

static void led_matrix_restore_persistent_statuses(void);
static void led_matrix_clear_transient_status_state(void);
static void led_matrix_clear_layer_color_runtime_state(void);
static void led_matrix_restore_base_runtime_overlays(void);
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
static void led_matrix_reset_auto_suspend_state(void);
static void led_matrix_schedule_idle_auto_off_if_needed(void);
#endif
static bool
led_matrix_activate_status_wake_context(enum led_matrix_status_wake_context wake_context);
static bool led_matrix_disable_force_show(void);
static void led_matrix_set_layer_color_state(const struct led_matrix_layer_color_cfg *cfg,
                                             bool persistent, uint32_t deadline_ms);
static void led_matrix_refresh_layer_color(bool trigger_timed_layer_color);
static int led_matrix_save_state(void);

static enum led_matrix_runtime_mode led_matrix_current_runtime_mode(void) {
    if (sleep_suspended) {
        return LED_MATRIX_RUNTIME_MODE_SLEEP_SUSPENDED;
    }

    if (state.on) {
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
        if (idle_auto_off_state.suspended) {
            return force_show_active ? LED_MATRIX_RUNTIME_MODE_AUTO_SUSPENDED_FORCE_SHOW
                                     : LED_MATRIX_RUNTIME_MODE_AUTO_SUSPENDED;
        }
#endif
        return LED_MATRIX_RUNTIME_MODE_ACTIVE;
    }

    return force_show_active ? LED_MATRIX_RUNTIME_MODE_FORCE_SHOW_ONLY
                             : LED_MATRIX_RUNTIME_MODE_OFF;
}

static const char *led_matrix_runtime_mode_name(enum led_matrix_runtime_mode mode) {
    switch (mode) {
    case LED_MATRIX_RUNTIME_MODE_ACTIVE:
        return "active";
    case LED_MATRIX_RUNTIME_MODE_FORCE_SHOW_ONLY:
        return "force-show";
    case LED_MATRIX_RUNTIME_MODE_AUTO_SUSPENDED:
        return "auto-suspended";
    case LED_MATRIX_RUNTIME_MODE_AUTO_SUSPENDED_FORCE_SHOW:
        return "auto-suspended-force-show";
    case LED_MATRIX_RUNTIME_MODE_SLEEP_SUSPENDED:
        return "sleep-suspended";
    case LED_MATRIX_RUNTIME_MODE_OFF:
    default:
        return "off";
    }
}

static bool led_matrix_runtime_mode_allows_base_render(enum led_matrix_runtime_mode mode) {
    return mode == LED_MATRIX_RUNTIME_MODE_ACTIVE;
}

static bool led_matrix_runtime_mode_allows_render(enum led_matrix_runtime_mode mode) {
    return mode == LED_MATRIX_RUNTIME_MODE_ACTIVE ||
           mode == LED_MATRIX_RUNTIME_MODE_FORCE_SHOW_ONLY ||
           mode == LED_MATRIX_RUNTIME_MODE_AUTO_SUSPENDED_FORCE_SHOW;
}

#if LED_MATRIX_BACKEND_HAS_MANAGED_POWER ||                                                        \
    (LED_DRIVER_IS_STRIP && IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EXT_POWER))
static void led_matrix_mark_runtime_output_wake(void) {
    runtime_first_frame_after_wake_pending = true;
    led_matrix_invalidate_flushed_frame();
}

static void led_matrix_clear_runtime_output_wake(void) {
    runtime_first_frame_after_wake_pending = false;
}
#endif

static inline bool led_matrix_is_auto_suspended(void) {
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
    return idle_auto_off_state.suspended;
#else
    return false;
#endif
}

static inline bool led_matrix_is_sleep_suspended(void) { return sleep_suspended; }

static inline bool led_matrix_base_render_enabled(void) {
    return led_matrix_runtime_mode_allows_base_render(led_matrix_current_runtime_mode());
}

static inline bool led_matrix_render_enabled(void) {
    return led_matrix_runtime_mode_allows_render(led_matrix_current_runtime_mode());
}

static inline bool led_matrix_runtime_output_has_physical_power_gating(void) {
#if LED_MATRIX_BACKEND_IS_COMPOSITE
    return (backend_composite_capabilities & ZMK_LED_COMPOSITE_CAP_MANAGED_POWER) != 0U;
#elif LED_MATRIX_BACKEND_HAS_MANAGED_POWER
    return true;
#elif LED_DRIVER_IS_STRIP && IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EXT_POWER)
    return true;
#else
    return false;
#endif
}

static void led_matrix_note_low_power_clear_only(enum led_matrix_runtime_mode runtime_mode) {
    if (runtime_mode == LED_MATRIX_RUNTIME_MODE_ACTIVE ||
        runtime_mode == LED_MATRIX_RUNTIME_MODE_FORCE_SHOW_ONLY ||
        runtime_mode == LED_MATRIX_RUNTIME_MODE_AUTO_SUSPENDED_FORCE_SHOW) {
        low_power_clear_notice_mode = LED_MATRIX_RUNTIME_MODE_ACTIVE;
        return;
    }

    if (led_matrix_runtime_output_has_physical_power_gating() ||
        low_power_clear_notice_mode == runtime_mode) {
        return;
    }

    LOG_DBG("LED matrix runtime %s cleared output without physical power gating; backend has no "
            "managed power",
            led_matrix_runtime_mode_name(runtime_mode));
    low_power_clear_notice_mode = runtime_mode;
}

#if LED_MATRIX_BACKEND_HAS_MANAGED_POWER
/* IS31 register programming lives in dedicated drivers; led_matrix.c only coordinates runtime
 * power state and wake sequencing.
 */
static inline const char *led_matrix_backend_name(void) {
#if LED_MATRIX_BACKEND_IS31FL3733
    return "IS31FL3733";
#elif LED_MATRIX_BACKEND_IS31FL3737
    return "IS31FL3737";
#elif LED_MATRIX_BACKEND_IS_COMPOSITE
    return "LED composite";
#else
    return "unknown";
#endif
}

#if LED_MATRIX_BACKEND_IS31FL3733 || LED_MATRIX_BACKEND_IS31FL3737
static inline bool led_matrix_backend_has_sdb(void) { return backend_sdb.port != NULL; }
#endif

static inline bool led_matrix_backend_has_full_hard_off(void) {
#if LED_MATRIX_BACKEND_IS_COMPOSITE
    return (backend_composite_capabilities & ZMK_LED_COMPOSITE_CAP_FULL_HARD_OFF) != 0U;
#elif LED_MATRIX_BACKEND_IS31FL3733 || LED_MATRIX_BACKEND_IS31FL3737
    return led_matrix_backend_has_sdb();
#else
    return false;
#endif
}

static inline bool led_matrix_backend_is_hard_off(void) {
    return backend_power_state_known && !backend_power_enabled &&
           backend_power_mode == LED_MATRIX_BACKEND_POWER_MODE_HARD_OFF;
}

static void led_matrix_backend_mark_unknown(void) {
    backend_power_state_known = false;
    backend_power_enabled = false;
    backend_power_mode = LED_MATRIX_BACKEND_POWER_MODE_SOFT;
    led_matrix_clear_runtime_output_wake();
}

#if LED_MATRIX_BACKEND_IS31FL3733 || LED_MATRIX_BACKEND_IS31FL3737
static int led_matrix_backend_blank(bool blank_en) {
    return zmk_is31_blank(&backend_is31_power, blank_en);
}
#endif

static int led_matrix_backend_set_soft_state(bool enable) {
    bool was_enabled = backend_power_state_known && backend_power_enabled;

#if LED_MATRIX_BACKEND_IS_COMPOSITE
    int err = zmk_led_composite_power(output_dev, enable);
    if (err != 0) {
        led_matrix_backend_mark_unknown();
        LOG_ERR("Unable to %s %s backend \"%s\": %d", enable ? "resume" : "soft-shutdown",
                led_matrix_backend_name(), output_dev->name, err);
        return err;
    }
#else
    int err = led_matrix_backend_blank(!enable);
    if (err != 0) {
        led_matrix_backend_mark_unknown();
        LOG_ERR("Unable to %s %s backend \"%s\": %d", enable ? "resume" : "soft-shutdown",
                led_matrix_backend_name(), output_dev->name, err);
        return err;
    }
#endif

    backend_power_state_known = true;
    backend_power_enabled = enable;
    backend_power_mode = LED_MATRIX_BACKEND_POWER_MODE_SOFT;

    if (enable && !was_enabled) {
        led_matrix_mark_runtime_output_wake();
    } else if (!enable) {
        led_matrix_clear_runtime_output_wake();
        led_matrix_invalidate_flushed_frame();
    } else {
        led_matrix_invalidate_flushed_frame();
    }

    return 0;
}

static int led_matrix_backend_enter_hard_off(void) {
    if (!led_matrix_backend_has_full_hard_off()) {
        return led_matrix_backend_set_soft_state(false);
    }

    if (led_matrix_backend_is_hard_off()) {
        return 0;
    }

    LOG_DBG("LED matrix hard-off enter: backend=%s runtime=%s", led_matrix_backend_name(),
            led_matrix_runtime_mode_name(led_matrix_current_runtime_mode()));

#if LED_MATRIX_BACKEND_IS_COMPOSITE
    int err = zmk_led_composite_power(output_dev, false);
    if (err != 0) {
        led_matrix_backend_mark_unknown();
        LOG_ERR("Unable to hard-shutdown %s backend \"%s\": %d", led_matrix_backend_name(),
                output_dev->name, err);
        return err;
    }
#else
    int err = zmk_is31_enter_hard_off(&backend_is31_power);
    if (err != 0) {
        led_matrix_backend_mark_unknown();
        LOG_ERR("Unable to hard-shutdown %s backend \"%s\": %d", led_matrix_backend_name(),
                output_dev->name, err);
        return err;
    }
#endif

    backend_power_state_known = true;
    backend_power_enabled = false;
    backend_power_mode = LED_MATRIX_BACKEND_POWER_MODE_HARD_OFF;
    led_matrix_clear_runtime_output_wake();
    led_matrix_invalidate_flushed_frame();
    return 0;
}

static int led_matrix_backend_resume_active(void) {
#if LED_MATRIX_BACKEND_IS_COMPOSITE
    int err = zmk_led_composite_power(output_dev, true);
    if (err != 0) {
        led_matrix_backend_mark_unknown();
        LOG_ERR("Unable to resume %s backend \"%s\": %d", led_matrix_backend_name(),
                output_dev->name, err);
        return err;
    }

    backend_power_state_known = true;
    backend_power_enabled = true;
    backend_power_mode = LED_MATRIX_BACKEND_POWER_MODE_SOFT;
    led_matrix_mark_runtime_output_wake();
    return 0;
#else
    if (led_matrix_backend_is_hard_off() || !backend_power_state_known) {
        LOG_DBG("LED matrix backend resume begin: backend=%s runtime=%s", led_matrix_backend_name(),
                led_matrix_runtime_mode_name(led_matrix_current_runtime_mode()));
        int err = zmk_is31_resume_from_hard_off(&backend_is31_power);
        if (err != 0) {
            led_matrix_backend_mark_unknown();
            LOG_ERR("LED matrix backend resume failure for \"%s\", backend marked unknown: %d",
                    output_dev->name, err);
            return err;
        }

        LOG_DBG("LED matrix backend resume end: backend=%s", led_matrix_backend_name());

        backend_power_state_known = true;
        backend_power_enabled = true;
        backend_power_mode = LED_MATRIX_BACKEND_POWER_MODE_SOFT;
        led_matrix_mark_runtime_output_wake();
        return 0;
    }

    return led_matrix_backend_set_soft_state(true);
#endif
}
#endif

static int led_matrix_sync_backend_power(bool should_enable) {
#if LED_MATRIX_BACKEND_HAS_MANAGED_POWER
    bool should_hard_off = !should_enable && led_matrix_backend_has_full_hard_off();

    if (should_enable) {
        if (backend_power_state_known && backend_power_enabled) {
            return 0;
        }

        return led_matrix_backend_resume_active();
    }

    if (backend_power_state_known && !backend_power_enabled &&
        ((!should_hard_off && backend_power_mode == LED_MATRIX_BACKEND_POWER_MODE_SOFT) ||
         (should_hard_off && backend_power_mode == LED_MATRIX_BACKEND_POWER_MODE_HARD_OFF))) {
        return 0;
    }

    if (should_hard_off) {
        return led_matrix_backend_enter_hard_off();
    }

    return led_matrix_backend_set_soft_state(false);
#else
    ARG_UNUSED(should_enable);
#endif

    return 0;
}

static int led_matrix_sync_ext_power(bool should_enable) {
#if LED_DRIVER_IS_STRIP && IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EXT_POWER)
    bool was_enabled = ext_power_state_known && ext_power_is_on;

    if (ext_power_state_known && ext_power_is_on == should_enable) {
        return 0;
    }

    int rc =
        should_enable ? ext_power_enable_runtime(ext_power) : ext_power_disable_runtime(ext_power);
    if (rc != 0) {
        ext_power_state_known = false;
        LOG_ERR("Unable to %s EXT_POWER: %d", should_enable ? "enable" : "disable", rc);
        return rc;
    }

    ext_power_state_known = true;
    ext_power_is_on = should_enable;
    if (should_enable && !was_enabled) {
        led_matrix_mark_runtime_output_wake();
    } else if (!should_enable) {
        led_matrix_clear_runtime_output_wake();
        led_matrix_invalidate_flushed_frame();
    } else {
        led_matrix_invalidate_flushed_frame();
    }
#else
    ARG_UNUSED(should_enable);
#endif

    return 0;
}

/* Composite success also includes cleared children without physical power gating. */
static bool led_matrix_runtime_output_is_quiesced(void) {
#if LED_MATRIX_BACKEND_HAS_MANAGED_POWER
    if (backend_power_state_known && !backend_power_enabled) {
        return true;
    }
#endif
#if LED_DRIVER_IS_STRIP && IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EXT_POWER)
    if (ext_power_state_known && !ext_power_is_on) {
        return true;
    }
#endif

    return false;
}

static int led_matrix_sync_active_runtime_output(void) {
    int err = led_matrix_sync_backend_power(true);
    if (err != 0) {
        return err;
    }

    err = led_matrix_sync_ext_power(true);
    if (err != 0) {
        /* A failed multi-pin enable may have supplied part of the strip rail. */
        int off_err = led_matrix_sync_ext_power(false);
        if (off_err != 0) {
            LOG_WRN("LED matrix external power resume rollback failed (%d)", off_err);
        }
        return err;
    }

    led_matrix_note_low_power_clear_only(led_matrix_current_runtime_mode());
    return 0;
}

static int led_matrix_clear_and_disable_runtime_output(void) {
    enum led_matrix_runtime_mode runtime_mode = led_matrix_current_runtime_mode();

    led_matrix_note_low_power_clear_only(runtime_mode);
    led_matrix_sync_tick_state();

    int err = 0;
    bool skip_flush = led_matrix_runtime_output_is_quiesced();

#if !LED_MATRIX_BACKEND_IS_COMPOSITE
    if (!skip_flush) {
        err = led_matrix_flush_cleared_output();
    }
#else
    /* The driver closes every child independently, including ungated outputs.
     * A whole-device clear here would write to already shut-down children on retry. */
    ARG_UNUSED(skip_flush);
#endif
    int clear_err = err;

    int power_err = led_matrix_sync_backend_power(false);

    err = led_matrix_sync_ext_power(false);
    if (power_err == 0) {
        power_err = err;
    }
    if (power_err != 0) {
        return power_err;
    }
    if (clear_err != 0 && led_matrix_runtime_output_is_quiesced()) {
        LOG_WRN("LED matrix clear failed (%d), but output shutdown completed", clear_err);
        return 0;
    }

    return clear_err;
}

static void led_matrix_restore_persistent_statuses(void) {
    ble_pairing_transient_active = false;
    ble_pairing_transient_allows_completion_overlay = false;

    led_matrix_refresh_automatic_persistent_status_sources();
}

static void led_matrix_clear_transient_status_state(void) {
    status_mode = STATUS_MODE_NONE;
    transient_sources = 0U;
    transient_deadline_ms = 0U;
    ble_pairing_transient_active = false;
    ble_pairing_transient_allows_completion_overlay = false;
    k_work_cancel_delayable(&led_matrix_clear_status_work);
}

static void led_matrix_clear_layer_color_runtime_state(void) {
    led_matrix_set_layer_color_state(NULL, false, 0U);
    k_work_cancel_delayable(&led_matrix_clear_layer_color_work);
}

static void led_matrix_clear_transient_status_state_sync(void) {
    struct k_work_sync sync;

    status_mode = STATUS_MODE_NONE;
    transient_sources = 0U;
    transient_deadline_ms = 0U;
    ble_pairing_transient_active = false;
    ble_pairing_transient_allows_completion_overlay = false;
    (void)k_work_cancel_delayable_sync(&led_matrix_clear_status_work, &sync);
}

static void led_matrix_clear_layer_color_runtime_state_sync(void) {
    struct k_work_sync sync;

    led_matrix_set_layer_color_state(NULL, false, 0U);
    (void)k_work_cancel_delayable_sync(&led_matrix_clear_layer_color_work, &sync);
}

static void led_matrix_restore_base_runtime_overlays(void) {
    if (!led_matrix_base_render_enabled()) {
        return;
    }

    led_matrix_refresh_layer_color(false);
    led_matrix_restore_persistent_statuses();
}

static void led_matrix_clear_runtime_suspend_state(bool synchronous) {
    force_show_active = false;
    force_show_wake_context = LED_MATRIX_WAKE_CONTEXT_NONE;
    ripple_state.active = false;
    led_matrix_reset_rain_state();

    if (synchronous) {
        led_matrix_clear_transient_status_state_sync();
        led_matrix_clear_layer_color_runtime_state_sync();
    } else {
        led_matrix_clear_transient_status_state();
        led_matrix_clear_layer_color_runtime_state();
    }

    clear_keypress_release_state();
}

static int led_matrix_suspend_runtime_output_async(uint8_t reason_mask) {
    led_matrix_clear_runtime_suspend_state(false);
    led_matrix_sync_tick_state();
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT);

    if (reason_mask != 0U) {
        (void)led_matrix_raise_state_changed(reason_mask);
    }

    return 0;
}

static int led_matrix_suspend_runtime_output_sync(uint8_t reason_mask) {
    struct k_work_sync render_sync;
    struct k_work_sync retry_sync;
    int err;

    led_matrix_clear_runtime_suspend_state(true);
    led_matrix_sync_tick_state();
    (void)atomic_inc(&led_matrix_output_generation);
    (void)k_work_cancel_delayable_sync(&led_matrix_output_retry_work, &retry_sync);
    (void)k_work_cancel_sync(&led_matrix_render_work, &render_sync);
    (void)atomic_clear(&led_matrix_service_requests);

    err = led_matrix_clear_and_disable_runtime_output();
    atomic_set(&led_matrix_output_sync_failed, err != 0);

    if (reason_mask != 0U) {
        (void)led_matrix_raise_state_changed(reason_mask);
    }

    return err;
}

static int led_matrix_resume_runtime_output(uint8_t reason_mask) {
    led_matrix_restore_base_runtime_overlays();
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER);

    if (reason_mask != 0U) {
        (void)led_matrix_raise_state_changed(reason_mask);
    }

    return 0;
}

#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
static void led_matrix_idle_auto_off_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    if (zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE || led_matrix_usb_powered() || !state.on ||
        led_matrix_is_auto_suspended() || led_matrix_is_sleep_suspended()) {
        return;
    }

    idle_auto_off_state.restore_on_wake = state.on;
    idle_auto_off_state.suspended = true;
    LOG_DBG("LED matrix idle suspend after status wake -> %s",
            led_matrix_runtime_mode_name(led_matrix_current_runtime_mode()));
    (void)led_matrix_suspend_runtime_output_async(ZMK_LED_MATRIX_STATE_CHANGE_ENABLED |
                                                  ZMK_LED_MATRIX_STATE_CHANGE_STATUS |
                                                  ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR);
}

K_WORK_DELAYABLE_DEFINE(led_matrix_idle_auto_off_work, led_matrix_idle_auto_off_work_handler);

static void led_matrix_cancel_idle_auto_off(void) {
    k_work_cancel_delayable(&led_matrix_idle_auto_off_work);
}

static void led_matrix_reset_auto_suspend_state(void) {
    idle_auto_off_state.suspended = false;
    idle_auto_off_state.restore_on_wake = false;
    led_matrix_cancel_idle_auto_off();
}
#else
static inline void led_matrix_reset_auto_suspend_state(void) {}
static inline void led_matrix_cancel_idle_auto_off(void) {}
static inline void led_matrix_schedule_idle_auto_off_if_needed(void) {}
static inline int led_matrix_auto_suspend(void) { return 0; }
static inline bool led_matrix_auto_resume_if_needed(void) { return false; }
#endif

static void led_matrix_note_idle_started(void) {
    if (idle_status_wake_state.idle_started_at_valid) {
        return;
    }

    idle_status_wake_state.idle_started_at_ms = k_uptime_get_32();
    idle_status_wake_state.idle_started_at_valid = true;
}

static void led_matrix_clear_idle_started(void) {
    idle_status_wake_state.idle_started_at_valid = false;
    idle_status_wake_state.idle_started_at_ms = 0U;
}

static bool led_matrix_idle_status_wake_timeout_expired(void) {
    if (zmk_activity_get_state() != ZMK_ACTIVITY_IDLE ||
        !idle_status_wake_state.idle_started_at_valid) {
        return false;
    }

    return (uint32_t)(k_uptime_get_32() - idle_status_wake_state.idle_started_at_ms) >=
           (uint32_t)CONFIG_ZMK_LED_MATRIX_IDLE_STATUS_WAKE_TIMEOUT;
}

#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE)
static void led_matrix_schedule_idle_auto_off_if_needed(void) {
    if (led_matrix_usb_powered() || !state.on || led_matrix_is_auto_suspended() ||
        led_matrix_is_sleep_suspended() || zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE) {
        led_matrix_cancel_idle_auto_off();
        return;
    }

    (void)led_matrix_reschedule_delayable_work(&led_matrix_idle_auto_off_work,
                                               K_MSEC(MAX(CONFIG_ZMK_IDLE_TIMEOUT, 1)));
}

static int led_matrix_auto_suspend(void) {
    if (!state.on || led_matrix_is_auto_suspended() || led_matrix_usb_powered() ||
        led_matrix_is_sleep_suspended()) {
        led_matrix_schedule_idle_auto_off_if_needed();
        return 0;
    }

    idle_auto_off_state.restore_on_wake = state.on;
    idle_auto_off_state.suspended = true;
    led_matrix_cancel_idle_auto_off();
    LOG_DBG("LED matrix idle suspend -> %s",
            led_matrix_runtime_mode_name(led_matrix_current_runtime_mode()));
    return led_matrix_suspend_runtime_output_async(ZMK_LED_MATRIX_STATE_CHANGE_ENABLED |
                                                   ZMK_LED_MATRIX_STATE_CHANGE_STATUS |
                                                   ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR);
}

static bool led_matrix_auto_resume_if_needed(void) {
    if (led_matrix_is_sleep_suspended() || !led_matrix_is_auto_suspended() ||
        !idle_auto_off_state.restore_on_wake || !state.on) {
        return false;
    }

    led_matrix_reset_auto_suspend_state();
    force_show_wake_context = LED_MATRIX_WAKE_CONTEXT_NONE;
    if (led_matrix_resume_runtime_output(ZMK_LED_MATRIX_STATE_CHANGE_ENABLED |
                                         ZMK_LED_MATRIX_STATE_CHANGE_STATUS |
                                         ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR) != 0) {
        return false;
    }

    led_matrix_schedule_idle_auto_off_if_needed();
    LOG_DBG("LED matrix idle resume -> %s",
            led_matrix_runtime_mode_name(led_matrix_current_runtime_mode()));
    return true;
}
#endif

static bool led_matrix_auto_status_wake_allowed(void) {
    if (led_matrix_is_sleep_suspended() || zmk_activity_get_state() == ZMK_ACTIVITY_SLEEP) {
        return false;
    }

    if (led_matrix_idle_status_wake_timeout_expired()) {
        LOG_DBG("LED matrix automatic status wake suppressed after idle timeout");
        return false;
    }

    return true;
}

static bool led_matrix_force_show_should_remain_active(void) {
    return !led_matrix_is_sleep_suspended() && !state.on && status_mode == STATUS_MODE_TRANSIENT;
}

static bool
led_matrix_activate_status_wake_context(enum led_matrix_status_wake_context wake_context) {
    if (led_matrix_is_sleep_suspended()) {
        return false;
    }

    if (wake_context == LED_MATRIX_WAKE_CONTEXT_NONE) {
        return led_matrix_base_render_enabled() || force_show_active;
    }

    if (force_show_active && force_show_wake_context == wake_context) {
        return true;
    }

    force_show_active = true;
    force_show_wake_context = wake_context;
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT);
    return true;
}

static bool led_matrix_enable_force_show_if_off(void) {
    return led_matrix_activate_status_wake_context(led_matrix_resolve_status_wake_context());
}

static bool led_matrix_disable_force_show(void) {
    if (!force_show_active || led_matrix_force_show_should_remain_active()) {
        return false;
    }

    force_show_active = false;
    force_show_wake_context = LED_MATRIX_WAKE_CONTEXT_NONE;
    if (!led_matrix_base_render_enabled()) {
        led_matrix_sync_tick_state();
        led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT);
        return true;
    }

    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER);
    return false;
}

static void led_matrix_set_layer_color_state(const struct led_matrix_layer_color_cfg *cfg,
                                             bool persistent, uint32_t deadline_ms) {
    layer_color.active = (cfg != NULL);
    layer_color.persistent = cfg != NULL ? persistent : false;
    layer_color.layer_id = cfg != NULL ? cfg->layer_id : ZMK_KEYMAP_LAYER_ID_INVAL;
    layer_color.deadline_ms = cfg != NULL ? deadline_ms : 0U;
    layer_color.cfg = cfg;
}

static void led_matrix_refresh_layer_color(bool trigger_timed_layer_color) {
    const struct led_matrix_layer_color_cfg *cfg = NULL;
    bool changed = false;

    if (led_matrix_base_render_enabled()) {
        cfg = find_layer_color_cfg(snapshot.active_layer);
    }

    if (cfg == NULL) {
        changed = layer_color.active;
        led_matrix_set_layer_color_state(NULL, false, 0U);
        k_work_cancel_delayable(&led_matrix_clear_layer_color_work);
    } else if (cfg->display_time_ms == 0U) {
        changed = !layer_color.active || layer_color.cfg != cfg || !layer_color.persistent;
        led_matrix_set_layer_color_state(cfg, true, 0U);
        k_work_cancel_delayable(&led_matrix_clear_layer_color_work);
    } else if (trigger_timed_layer_color) {
        uint32_t deadline_ms = k_uptime_get_32() + cfg->display_time_ms;
        changed = !layer_color.active || layer_color.cfg != cfg || layer_color.persistent ||
                  layer_color.deadline_ms != deadline_ms;
        led_matrix_set_layer_color_state(cfg, false, deadline_ms);
        (void)led_matrix_reschedule_delayable_work(&led_matrix_clear_layer_color_work,
                                                   K_MSEC(cfg->display_time_ms));
    } else {
        changed = layer_color.active;
        led_matrix_set_layer_color_state(NULL, false, 0U);
        k_work_cancel_delayable(&led_matrix_clear_layer_color_work);
    }

    if (changed) {
        (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR);
    }
}

static void led_matrix_clear_status_handler(struct k_work *work) {
    ARG_UNUSED(work);
    bool was_ble_pairing_transient = ble_pairing_transient_active;

    if (status_mode != STATUS_MODE_TRANSIENT) {
        return;
    }

    status_mode = STATUS_MODE_NONE;
    transient_sources = 0U;
    transient_deadline_ms = 0U;
    ble_pairing_transient_active = false;
    ble_pairing_transient_allows_completion_overlay = false;
    bool restored_off = led_matrix_disable_force_show();
    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_STATUS);

    if (was_ble_pairing_transient) {
        LOG_DBG("LED matrix BLE pairing transient expired");
    }

    LOG_DBG("LED matrix transient timeout expired, restoring lower-priority output%s",
            restored_off ? " and power-off output" : "");

    if (restored_off) {
        return;
    }

    led_matrix_schedule_render();
}

K_WORK_DELAYABLE_DEFINE(led_matrix_clear_status_work, led_matrix_clear_status_handler);

static void led_matrix_clear_layer_color_handler(struct k_work *work) {
    ARG_UNUSED(work);

    if (!layer_color.active || layer_color.persistent) {
        return;
    }

    led_matrix_set_layer_color_state(NULL, false, 0U);
    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR);

    if (led_matrix_base_render_enabled()) {
        led_matrix_schedule_render();
    }
}

K_WORK_DELAYABLE_DEFINE(led_matrix_clear_layer_color_work, led_matrix_clear_layer_color_handler);

#if IS_ENABLED(CONFIG_SETTINGS)
static void led_matrix_save_state_handler(struct k_work *work) {
    ARG_UNUSED(work);
    struct led_matrix_settings_v5 persisted = {
        .on = state.on ? 1U : 0U,
        .brightness = state.brightness,
        .selected_profile_id = state.selected_profile_id,
        .speed = state.speed,
        .manual_hue_index = state.manual_hue_index,
    };

    led_matrix_store_selected_independent_profiles(
        persisted.selected_independent_profile_ids,
        ARRAY_SIZE(persisted.selected_independent_profile_ids));

    LOG_DBG("LED matrix state save start: debounce=%d bytes=%u ids=%u on=%d brt=%u profile=%u "
            "speed=%u hue=%u",
            CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE, (unsigned int)sizeof(persisted),
            (unsigned int)ARRAY_SIZE(persisted.selected_independent_profile_ids), persisted.on,
            persisted.brightness, persisted.selected_profile_id, persisted.speed,
            persisted.manual_hue_index);

    int rc = settings_save_one("led_matrix/state", &persisted, sizeof(persisted));
    if (rc != 0) {
        LOG_ERR("LED matrix state save failed: rc=%d bytes=%u ids=%u on=%d brt=%u profile=%u "
                "speed=%u hue=%u",
                rc, (unsigned int)sizeof(persisted),
                (unsigned int)ARRAY_SIZE(persisted.selected_independent_profile_ids), persisted.on,
                persisted.brightness, persisted.selected_profile_id, persisted.speed,
                persisted.manual_hue_index);
        return;
    }

    LOG_DBG("LED matrix state save success: bytes=%u ids=%u on=%d brt=%u profile=%u speed=%u "
            "hue=%u",
            (unsigned int)sizeof(persisted),
            (unsigned int)ARRAY_SIZE(persisted.selected_independent_profile_ids), persisted.on,
            persisted.brightness, persisted.selected_profile_id, persisted.speed,
            persisted.manual_hue_index);
}

static struct k_work_delayable led_matrix_save_work;
#endif

static int led_matrix_save_state(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &led_matrix_save_work,
                                          K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));

    if (ret < 0) {
        LOG_ERR("LED matrix state save schedule failed: rc=%d debounce=%d on=%d brt=%u profile=%u "
                "speed=%u hue=%u",
                ret, CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE, state.on ? 1 : 0, state.brightness,
                state.selected_profile_id, state.speed, state.manual_hue_index);
        return ret;
    }

    LOG_DBG("LED matrix state save scheduled: debounce=%d ids=%u on=%d brt=%u profile=%u speed=%u "
            "hue=%u",
            CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE,
            (unsigned int)ARRAY_SIZE(selected_independent_profile_ids), state.on ? 1 : 0,
            state.brightness, state.selected_profile_id, state.speed, state.manual_hue_index);
    return 0;
#else
    return 0;
#endif
}
