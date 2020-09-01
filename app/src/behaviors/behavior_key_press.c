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

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_key_press_config {
    u8_t usage_page;
};
struct behavior_key_press_data {};

static int behavior_key_press_init(struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t keycode, u32_t _,
                                     s64_t timestamp) {
    const struct behavior_key_press_config *cfg = dev->config_info;
    LOG_DBG("position %d usage_page 0x%02X keycode 0x%02X", position, cfg->usage_page, keycode);

    return ZMK_EVENT_RAISE(
        create_keycode_state_changed(cfg->usage_page, keycode, true, position, timestamp));
}

static int on_keymap_binding_released(struct device *dev, u32_t position, u32_t keycode, u32_t _,
                                      s64_t timestamp) {
    const struct behavior_key_press_config *cfg = dev->config_info;
    LOG_DBG("position %d usage_page 0x%02X keycode 0x%02X", position, cfg->usage_page, keycode);

    return ZMK_EVENT_RAISE(
        create_keycode_state_changed(cfg->usage_page, keycode, false, position, timestamp));
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