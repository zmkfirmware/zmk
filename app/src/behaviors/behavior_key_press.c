/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_key_press

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/event-manager.h>
#include <zmk/events/keycode-state-changed.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_key_press_config {
    u8_t usage_page;
};
struct behavior_key_press_data {};

static int behavior_key_press_init(struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_key_press_config *cfg = dev->config_info;
    LOG_DBG("position %d usage_page 0x%02X keycode 0x%02X", event.position, cfg->usage_page,
            binding->param1);

    return ZMK_EVENT_RAISE(create_keycode_state_changed(cfg->usage_page, binding->param1, true));
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_key_press_config *cfg = dev->config_info;
    LOG_DBG("position %d usage_page 0x%02X keycode 0x%02X", event.position, cfg->usage_page,
            binding->param1);

    return ZMK_EVENT_RAISE(create_keycode_state_changed(cfg->usage_page, binding->param1, false));
}

static const struct behavior_driver_api behavior_key_press_driver_api = {
    .binding_pressed = on_keymap_binding_pressed, .binding_released = on_keymap_binding_released};

#define KP_INST(n)                                                                                 \
    static const struct behavior_key_press_config behavior_key_press_config_##n = {                \
        .usage_page = DT_INST_PROP(n, usage_page)};                                                \
    static struct behavior_key_press_data behavior_key_press_data_##n;                             \
    DEVICE_AND_API_INIT(behavior_key_press_##n, DT_INST_LABEL(n), behavior_key_press_init,         \
                        &behavior_key_press_data_##n, &behavior_key_press_config_##n, APPLICATION, \
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_key_press_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)