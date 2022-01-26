/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sarcasm

#include <kernel.h>
#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_sarcasm_config {
    uint8_t index;
};

struct behavior_sarcasm_data {
    bool active;
};

static void activate_sarcasm(const struct device *dev) {
    struct behavior_sarcasm_data *data = dev->data;

    data->active = true;
}

static void deactivate_sarcasm(const struct device *dev) {
    struct behavior_sarcasm_data *data = dev->data;

    data->active = false;
}

static int on_sarcasm_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    struct behavior_sarcasm_data *data = dev->data;

    if (data->active) {
        deactivate_sarcasm(dev);
    } else {
        activate_sarcasm(dev);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_sarcasm_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_sarcasm_driver_api = {
    .binding_pressed = on_sarcasm_binding_pressed,
    .binding_released = on_sarcasm_binding_released,
};

static int sarcasm_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_sarcasm, sarcasm_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_sarcasm, zmk_keycode_state_changed);

static const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

static bool sarcasm_is_alpha(uint8_t usage_id) {
    return (usage_id >= HID_USAGE_KEY_KEYBOARD_A && usage_id <= HID_USAGE_KEY_KEYBOARD_Z);
}

static void sarcasm_enhance_usage(const struct behavior_sarcasm_config *config,
                                  struct zmk_keycode_state_changed *ev) {
    if (ev->usage_page != HID_USAGE_KEY || !sarcasm_is_alpha(ev->keycode)) {
        return;
    }

    LOG_DBG("Enhancing usage 0x%02X with sarcasm", ev->keycode);
    ev->implicit_modifiers |= MOD_LSFT;
}

static int sarcasm_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); i++) {
        const struct device *dev = devs[i];
        if (dev == NULL) {
            continue;
        }

        struct behavior_sarcasm_data *data = dev->data;
        if (!data->active) {
            continue;
        }

        const struct behavior_sarcasm_config *config = dev->config;

        if (zmk_hid_get_explicit_mods() == 0 && k_uptime_get() % 2 == 1) {
            sarcasm_enhance_usage(config, ev);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int behavior_sarcasm_init(const struct device *dev) {
    const struct behavior_sarcasm_config *config = dev->config;
    devs[config->index] = dev;
    return 0;
}

#define KP_INST(n)                                                                                 \
    static struct behavior_sarcasm_data behavior_sarcasm_data_##n = {.active = false};             \
    static struct behavior_sarcasm_config behavior_sarcasm_config_##n = {                          \
        .index = n,                                                                                \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, behavior_sarcasm_init, device_pm_control_nop,                         \
                          &behavior_sarcasm_data_##n, &behavior_sarcasm_config_##n, APPLICATION,   \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_sarcasm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
