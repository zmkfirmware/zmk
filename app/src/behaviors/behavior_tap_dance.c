/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_tap_dance

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>

#include <zmk/matrix.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static bool tap_dance_started = false;
static int tap_counter = 0; 
int64_t timestamp;

struct behavior_tap_dance_config {    
    int tapping_term_ms;
};

static int on_tap_dance_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_tap_dance_config *cfg = dev->config;
    if (!tap_dance_started) {
        tap_dance_started = true;
        timestamp = k_uptime_get();
        LOG_DBG("timestamp at: %lld", timestamp);
    }
    else {
        if (k_uptime_get() <= (timestamp + cfg->tapping_term_ms)){
            timestamp = k_uptime_get();
            LOG_DBG("Updated timestamp: %lld", timestamp);
        }
    }
    int32_t tapping_term_ms_left = (timestamp + cfg->tapping_term_ms) - k_uptime_get();
    LOG_DBG("time left: %d", tapping_term_ms_left);

    if (tapping_term_ms_left <= 0) {
        LOG_DBG("TIME'S UP");
        LOG_DBG("Counter reached on press: %d", tap_counter);
        tap_dance_started = false;
        tap_counter = 0;
    }
    else {
        ++ tap_counter;
    }             
    LOG_DBG("Counter beginning at: %d", tap_counter);
    

    return ZMK_BEHAVIOR_OPAQUE;

    
}

static int on_tap_dance_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_tap_dance_config *cfg = dev->config;
    int32_t tapping_term_ms_left = (timestamp + cfg->tapping_term_ms) - k_uptime_get();
    if (tapping_term_ms_left <= 0) {
        LOG_DBG("Counter reached on release: %d", tap_counter);
        tap_dance_started = false;
        tap_counter = 0;
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_tap_dance_driver_api = {
    .binding_pressed = on_tap_dance_binding_pressed,
    .binding_released = on_tap_dance_binding_released,
};

static int behavior_tap_dance_init(const struct device *dev) { return 0; };

struct behavior_tap_dance_data {};
static struct behavior_tap_dance_data behavior_tap_dance_data;

#define KP_INST(n)                                                                                                         \
    static struct behavior_tap_dance_config behavior_tap_dance_config_##n = {                                              \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                                                                                           \
    };                                                                                                                     \
    DEVICE_AND_API_INIT(behavior_tap_dance_##n, DT_INST_LABEL(n), behavior_tap_dance_init, &behavior_tap_dance_data,       \
                        &behavior_tap_dance_config_##n, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                        &behavior_tap_dance_driver_api);
    
DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif