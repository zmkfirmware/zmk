/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_midi_key_press

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/midi_key_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_midi_key_press_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);

    return raise_zmk_midi_key_state_changed_from_encoded(binding->param1, true, event.timestamp);
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);
    return raise_zmk_midi_key_state_changed_from_encoded(binding->param1, false, event.timestamp);
}

static const struct behavior_driver_api behavior_midi_key_press_driver_api = {
    .binding_pressed = on_keymap_binding_pressed, .binding_released = on_keymap_binding_released};

#define MIDI_INST(n)                                                                               \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_midi_key_press_init, NULL, NULL, NULL, POST_KERNEL,        \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_midi_key_press_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIDI_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */