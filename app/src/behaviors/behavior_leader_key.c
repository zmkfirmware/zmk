/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_leader_key

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior.h>
#include <zmk/leader.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_leader_key_config {
    int32_t timeout_ms;
    bool timerless;
};

static int behavior_leader_key_init(const struct device *dev) { return 0; }

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_leader_key_config *cfg = dev->config;

    zmk_leader_activate(cfg->timeout_ms, cfg->timerless, event.position);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return 0;
}

static const struct behavior_driver_api behavior_leader_key_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define LEAD_INST(n)                                                                               \
    static struct behavior_leader_key_config behavior_leader_key_config_##n = {                    \
        .timerless = DT_INST_PROP(n, timerless), .timeout_ms = DT_INST_PROP(n, timeout_ms)};       \
    DEVICE_DT_INST_DEFINE(n, behavior_leader_key_init, NULL, NULL,                                 \
                          &behavior_leader_key_config_##n, APPLICATION,                            \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_leader_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LEAD_INST)
