/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "internal.h"

#if IS_ENABLED(CONFIG_SETTINGS)
static int led_matrix_settings_load_cb(const char *name, size_t len, settings_read_cb read_cb,
                                       void *cb_arg) {
    const char *next;
    if (!settings_name_steq(name, "state", &next) || next != NULL) {
        return -ENOENT;
    }

    led_matrix_reset_selected_independent_profiles();

    if (len == sizeof(struct led_matrix_settings_v1)) {
        struct led_matrix_settings_v1 loaded;
        int rc = read_cb(cb_arg, &loaded, sizeof(loaded));
        if (rc < 0) {
            return rc;
        }

        state.on = loaded.on != 0U;
        state.brightness = loaded.brightness;
        state.selected_profile_id = loaded.selected_profile_id;
        state.speed = led_matrix_default_speed();
        state.manual_hue_index = led_matrix_default_hue_index();
    } else if (len == sizeof(struct led_matrix_settings_v2)) {
        struct led_matrix_settings_v2 loaded;
        int rc = read_cb(cb_arg, &loaded, sizeof(loaded));
        if (rc < 0) {
            return rc;
        }

        state.on = loaded.on != 0U;
        state.brightness = loaded.brightness;
        state.selected_profile_id = loaded.selected_profile_id;
        state.speed = loaded.speed;
        state.manual_hue_index = led_matrix_default_hue_index();
    } else if (len == sizeof(struct led_matrix_settings_v3)) {
        struct led_matrix_settings_v3 loaded;
        int rc = read_cb(cb_arg, &loaded, sizeof(loaded));
        if (rc < 0) {
            return rc;
        }

        state.on = loaded.on != 0U;
        state.brightness = loaded.brightness;
        state.selected_profile_id = loaded.selected_profile_id;
        state.speed = loaded.speed;
        state.manual_hue_index = loaded.manual_hue_index;
    } else if (len == sizeof(struct led_matrix_settings_v4)) {
        struct led_matrix_settings_v4 loaded;
        int rc = read_cb(cb_arg, &loaded, sizeof(loaded));
        if (rc < 0) {
            return rc;
        }

        state.on = loaded.on != 0U;
        state.brightness = loaded.brightness;
        state.selected_profile_id = loaded.selected_profile_id;
        state.speed = loaded.speed;
        state.manual_hue_index = loaded.manual_hue_index;
        led_matrix_load_selected_independent_profiles(
            loaded.selected_independent_profile_ids,
            ARRAY_SIZE(loaded.selected_independent_profile_ids));
    } else if (len == sizeof(struct led_matrix_settings_v5)) {
        struct led_matrix_settings_v5 loaded;
        int rc = read_cb(cb_arg, &loaded, sizeof(loaded));
        if (rc < 0) {
            return rc;
        }

        state.on = loaded.on != 0U;
        state.brightness = loaded.brightness;
        state.selected_profile_id = loaded.selected_profile_id;
        state.speed = loaded.speed;
        state.manual_hue_index = loaded.manual_hue_index;
        led_matrix_load_selected_independent_profiles(
            loaded.selected_independent_profile_ids,
            ARRAY_SIZE(loaded.selected_independent_profile_ids));
    } else {
        return -EINVAL;
    }

    led_matrix_sanitize_runtime_state();

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(led_matrix, "led_matrix", NULL, led_matrix_settings_load_cb, NULL,
                               NULL);
#endif

static int zmk_led_matrix_init(void) {
    if (!device_is_ready(output_dev)) {
        LOG_ERR("LED matrix output device \"%s\" is not ready", output_dev->name);
        return -ENODEV;
    }

#if LED_MATRIX_BACKEND_IS31FL3733 || LED_MATRIX_BACKEND_IS31FL3737
    if (!i2c_is_ready_dt(&backend_i2c)) {
        LOG_ERR("%s I2C bus for \"%s\" is not ready", led_matrix_backend_name(), output_dev->name);
        return -ENODEV;
    }

    if (led_matrix_backend_has_sdb() && !gpio_is_ready_dt(&backend_sdb)) {
        LOG_ERR("%s SDB GPIO for \"%s\" is not ready", led_matrix_backend_name(), output_dev->name);
        return -ENODEV;
    }
#elif LED_MATRIX_BACKEND_IS_COMPOSITE
    uint32_t composite_channel_count;
    int composite_err = zmk_led_composite_get_channel_count(output_dev, &composite_channel_count);
    if (composite_err != 0) {
        LOG_ERR("Unable to query composite LED backend \"%s\": %d", output_dev->name,
                composite_err);
        return composite_err;
    }

    if (composite_channel_count != OUTPUT_CHANNEL_COUNT) {
        LOG_ERR("Composite LED backend \"%s\" exposes %u channels but zmk,led-matrix expects %u",
                output_dev->name, composite_channel_count, OUTPUT_CHANNEL_COUNT);
        return -EINVAL;
    }

    composite_err = zmk_led_composite_get_capabilities(output_dev, &backend_composite_capabilities);
    if (composite_err != 0) {
        LOG_ERR("Unable to query composite LED capabilities for \"%s\": %d", output_dev->name,
                composite_err);
        return composite_err;
    }
#endif

#if LED_DRIVER_IS_STRIP
    size_t strip_len = led_strip_length(output_dev);
    if (strip_len < OUTPUT_PIXEL_COUNT) {
        LOG_ERR("LED strip length (%d) is smaller than output pixel count (%d)", (int)strip_len,
                (int)OUTPUT_PIXEL_COUNT);
        return -EINVAL;
    }
#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EXT_POWER)
    if (!device_is_ready(ext_power)) {
        LOG_ERR("External power device \"%s\" is not ready", ext_power->name);
        return -ENODEV;
    }
#endif
#endif

    int err = validate_dts_layout();
    if (err != 0) {
        LOG_ERR("Invalid zmk,led-matrix DTS configuration (%d)", err);
        return err;
    }

#if LED_DRIVER_IS_MATRIX
    err = configure_matrix_output_sink();
    if (err != 0) {
        return err;
    }
#endif

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&led_matrix_save_work, led_matrix_save_state_handler);
#endif

    snapshot = (struct led_matrix_snapshot){0};
    led_matrix_refresh_status_snapshot(&snapshot);
    led_matrix_refresh_layer_snapshot();

    led_matrix_sanitize_runtime_state();
    ripple_state.active = false;
    led_matrix_reset_rain_state();
    ble_pairing_transient_active = false;
    ble_pairing_transient_allows_completion_overlay = false;
    sleep_suspended = zmk_activity_get_state() == ZMK_ACTIVITY_SLEEP;
    memset(pressed_positions, 0, sizeof(pressed_positions));
    clear_keypress_release_state();
    led_matrix_reset_auto_suspend_state();
    led_matrix_invalidate_flushed_frame();
    led_matrix_tick_running = false;
    runtime_first_frame_after_wake_pending = false;

    led_matrix_restore_base_runtime_overlays();

    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT);
    if (led_matrix_render_enabled()) {
        led_matrix_schedule_render();
    }

    LOG_INF("Initialized LED matrix: leds=%d independent=%d backend=%s sink=%s channels=%d "
            "output-channels=%d profiles=%d layer-colors=%d local-grid=%ux%u cells=%d",
            LOGICAL_LED_COUNT, (int)led_matrix_total_independent_led_count(),
            LED_DRIVER_IS_MATRIX ? "matrix" : "strip",
#if LED_DRIVER_IS_MATRIX
            matrix_output_sink.name,
#else
            "strip",
#endif
            RGB_CHANNEL_COUNT_PER_LED, OUTPUT_CHANNEL_COUNT, (int)DISPLAY_PROFILE_COUNT,
            (int)LAYER_COLOR_COUNT, grid_row_count, grid_col_count, (int)grid_cell_count);
    LOG_INF("LED matrix topology: grid=%s", led_matrix_has_spatial_grid() ? "present" : "absent");
    LOG_INF("LED matrix status modes: battery=%s usb=%s ble=%s hid=%s sleep=%s",
            timeout_for_source(LED_MATRIX_STATUS_BATTERY) == 0U ? "persistent" : "timed",
            timeout_for_source(LED_MATRIX_STATUS_USB) == 0U ? "persistent" : "timed",
            timeout_for_source(LED_MATRIX_STATUS_BLE) == 0U ? "persistent" : "timed",
            timeout_for_source(LED_MATRIX_STATUS_HID) == 0U ? "persistent" : "timed",
            sleep_suspended ? "suspended" : "active");
    LOG_DBG("LED matrix idle status wake timeout: %dms",
            CONFIG_ZMK_LED_MATRIX_IDLE_STATUS_WAKE_TIMEOUT);
    return 0;
}

int zmk_led_matrix_on(void) {
    state.brightness = MAX(state.brightness, (uint8_t)CONFIG_ZMK_LED_MATRIX_BRT_STEP);
    state.on = true;
    force_show_active = false;
    force_show_wake_context = LED_MATRIX_WAKE_CONTEXT_NONE;
    led_matrix_reset_auto_suspend_state();
    led_matrix_restore_base_runtime_overlays();
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER);
    led_matrix_schedule_idle_auto_off_if_needed();
    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_ENABLED |
                                         ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR |
                                         ZMK_LED_MATRIX_STATE_CHANGE_STATUS);
    return led_matrix_save_state();
}

int zmk_led_matrix_off(void) {
    led_matrix_reset_auto_suspend_state();
    state.on = false;
    led_matrix_refresh_automatic_persistent_status_sources();
    (void)led_matrix_suspend_runtime_output_async(ZMK_LED_MATRIX_STATE_CHANGE_ENABLED |
                                                  ZMK_LED_MATRIX_STATE_CHANGE_STATUS |
                                                  ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR);
    return led_matrix_save_state();
}

int zmk_led_matrix_toggle(void) { return state.on ? zmk_led_matrix_off() : zmk_led_matrix_on(); }

bool zmk_led_matrix_is_on(void) { return state.on; }

int zmk_led_matrix_set_brt(uint8_t brightness) {
    uint8_t new_brightness = MIN(brightness, (uint8_t)100);
    bool was_off = !state.on;

    if (new_brightness == 0U) {
        return zmk_led_matrix_off();
    }

    state.brightness = new_brightness;
    state.on = true;
    if (was_off) {
        force_show_active = false;
    }
    force_show_wake_context = LED_MATRIX_WAKE_CONTEXT_NONE;
    led_matrix_reset_auto_suspend_state();
    led_matrix_restore_base_runtime_overlays();

    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER);
    led_matrix_schedule_idle_auto_off_if_needed();
    (void)led_matrix_raise_state_changed(
        ZMK_LED_MATRIX_STATE_CHANGE_ENABLED | ZMK_LED_MATRIX_STATE_CHANGE_BRIGHTNESS |
        ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR | ZMK_LED_MATRIX_STATE_CHANGE_STATUS);
    return led_matrix_save_state();
}

uint8_t zmk_led_matrix_get_brt(void) { return state.on ? state.brightness : 0U; }

uint8_t zmk_led_matrix_calc_brt(int direction) {
    int brt = state.brightness + (direction * CONFIG_ZMK_LED_MATRIX_BRT_STEP);
    return CLAMP(brt, 0, 100);
}

int zmk_led_matrix_set_spd(uint8_t speed) {
    uint8_t new_speed = led_matrix_normalize_speed(speed);

    if (state.speed == new_speed) {
        return 0;
    }

    state.speed = new_speed;
    if (led_matrix_render_enabled()) {
        led_matrix_schedule_render();
    }

    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_SPEED);
    return led_matrix_save_state();
}

uint8_t zmk_led_matrix_get_spd(void) { return state.speed; }

uint8_t zmk_led_matrix_calc_spd(int direction) {
    int speed = zmk_led_matrix_get_spd() + direction;
    return CLAMP(speed, LED_MATRIX_SPEED_MIN, LED_MATRIX_SPEED_MAX);
}

int zmk_led_matrix_set_hue_index(uint8_t index) {
    uint8_t new_index = led_matrix_normalize_hue_index(index);

    if (state.manual_hue_index == new_index) {
        return 0;
    }

    state.manual_hue_index = new_index;
    if (led_matrix_render_enabled()) {
        led_matrix_schedule_render();
    }

    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_COLOR);
    return led_matrix_save_state();
}

uint8_t zmk_led_matrix_get_hue_index(void) { return state.manual_hue_index; }

uint8_t zmk_led_matrix_calc_hue_index(int direction) {
    int next = (int)zmk_led_matrix_get_hue_index() + direction;
    int steps = (int)LED_MATRIX_HUE_INDEX_MAX + 1;

    while (next < 0) {
        next += steps;
    }

    return (uint8_t)(next % steps);
}

int zmk_led_matrix_set_profile(uint8_t profile_id) {
    if (!led_matrix_has_profiles()) {
        LOG_DBG("LED matrix profile command ignored: no profiles configured");
        return 0;
    }

    if (find_display_profile(profile_id) == NULL) {
        return -EINVAL;
    }

    state.selected_profile_id = profile_id;
    ripple_state.active = false;
    led_matrix_reset_rain_state();
    clear_keypress_release_state();
    led_matrix_schedule_render();
    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_PROFILE);
    return led_matrix_save_state();
}

uint8_t zmk_led_matrix_get_profile(void) {
    const struct led_matrix_profile_cfg *profile = selected_profile_cfg();
    return profile != NULL ? profile->profile_id : 0U;
}

uint8_t zmk_led_matrix_calc_profile(int direction) {
#if DISPLAY_PROFILE_COUNT == 0
    ARG_UNUSED(direction);
    return 0U;
#else
    if (!led_matrix_has_profiles()) {
        return 0U;
    }

    int next = (int)zmk_led_matrix_get_profile() + direction;

    while (next < 0) {
        next += DISPLAY_PROFILE_COUNT;
    }

    return (uint8_t)(next % DISPLAY_PROFILE_COUNT);
#endif
}

int zmk_led_matrix_set_independent_profile(uint8_t child_id, uint8_t profile_id) {
    if (!led_matrix_has_independent_groups()) {
        LOG_DBG("LED matrix child profile command ignored: no independent children configured");
        return 0;
    }

    const struct led_matrix_independent_group_cfg *group = find_independent_group(child_id);
    if (group == NULL || find_independent_profile(group, profile_id) == NULL) {
        return -EINVAL;
    }

    if (selected_independent_profile_ids[child_id] == profile_id) {
        return 0;
    }

    selected_independent_profile_ids[child_id] = profile_id;
    if (led_matrix_render_enabled()) {
        led_matrix_schedule_render();
    }

    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_CHILD_PROFILE);
    return led_matrix_save_state();
}

uint8_t zmk_led_matrix_get_independent_profile(uint8_t child_id) {
    const struct led_matrix_independent_group_cfg *group = find_independent_group(child_id);

    if (group == NULL) {
        return 0U;
    }

    const struct led_matrix_independent_profile_cfg *profile =
        selected_independent_profile_cfg(group);
    return profile != NULL ? profile->profile_id : 0U;
}

uint8_t zmk_led_matrix_calc_independent_profile(uint8_t child_id, int direction) {
    const struct led_matrix_independent_group_cfg *group = find_independent_group(child_id);

    if (group == NULL || group->profile_count == 0U) {
        return 0U;
    }

    int next = (int)zmk_led_matrix_get_independent_profile(child_id) + direction;

    while (next < 0) {
        next += (int)group->profile_count;
    }

    return (uint8_t)(next % group->profile_count);
}

int zmk_led_matrix_get_state(struct zmk_led_matrix_state *out) {
    if (out == NULL) {
        return -EINVAL;
    }

    const struct led_matrix_profile_cfg *profile = selected_profile_cfg();
    uint8_t selected_profile_id = profile != NULL ? profile->profile_id : 0U;
    const char *selected_profile_name = profile != NULL ? profile->display_name : "";
    const char *selected_renderer_name = profile != NULL ? renderer_name(profile->renderer) : "";
    enum zmk_led_matrix_render_source source = led_matrix_effective_source();
    uint8_t effective_status_sources = current_status_sources();

    *out = (struct zmk_led_matrix_state){
        .enabled = state.on,
        .brightness = state.brightness,
        .effective_source = source,
        .selected_profile_id = selected_profile_id,
        .selected_profile_name = selected_profile_name,
        .selected_renderer_name = selected_renderer_name,
        .effective_status_sources_mask =
            (led_matrix_render_enabled() && effective_status_sources != 0U)
                ? effective_status_sources
                : 0U,
        .effective_layer_color_layer_id =
            (led_matrix_base_render_enabled() && layer_color.active && layer_color.cfg != NULL)
                ? layer_color.layer_id
                : ZMK_KEYMAP_LAYER_ID_INVAL,
    };

    return 0;
}

static uint8_t led_matrix_status_mask_to_sources(uint8_t mask) {
    uint8_t sources = 0U;

    if ((mask & LM_STA_BATTERY) != 0U) {
        sources |= LED_MATRIX_STATUS_BATTERY;
    }
    if ((mask & LM_STA_USB) != 0U) {
        sources |= LED_MATRIX_STATUS_USB;
    }
    if ((mask & LM_STA_BLE) != 0U) {
        sources |= LED_MATRIX_STATUS_BLE;
    }
    if ((mask & LM_STA_HID) != 0U) {
        sources |= LED_MATRIX_STATUS_HID;
    }

    return sources == 0U ? LED_MATRIX_STATUS_ALL : sources;
}

int zmk_led_matrix_show_status(uint8_t sources_mask) {
    if (ble_pairing_transient_active) {
        LOG_DBG("Ignore manual status request during BLE pairing transient");
        return 0;
    }

    led_matrix_refresh_status_snapshot(&snapshot);
    led_matrix_refresh_automatic_persistent_status_sources();
    led_matrix_enable_force_show_if_off();
    led_matrix_start_status_overlay(led_matrix_status_mask_to_sources(sources_mask),
                                    STATUS_TIMEOUT_MS, false);
    led_matrix_schedule_render();
    return 0;
}

SYS_INIT(zmk_led_matrix_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
