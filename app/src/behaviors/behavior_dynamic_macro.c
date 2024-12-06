/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_dynamic_macro

#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/dynamic_macro.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct k_timer dynamic_macro_work_timer;

enum behavior_dynamic_macro_states {
    DYNAMIC_MACRO_STATE_STOPPED = 0,
    DYNAMIC_MACRO_STATE_RECORDING,
    DYNAMIC_MACRO_STATE_PLAYING,
};

struct behavior_dynamic_macro_slot {
    uint8_t state;
    uint32_t current_event;
    uint32_t event_count;
    struct zmk_keycode_state_changed events[CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_EVENTS];
};

struct behavior_dynamic_macro_data {
    struct behavior_dynamic_macro_slot slots[CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_SLOTS];
} dynamic_macro_data;

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata cmd_param1_values[] = {
    {
        .display_name = "Start recording macro",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = DM_REC,
    },
    {
        .display_name = "Play macro",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = DM_PLY,
    },
    {
        .display_name = "Stop recording macro",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = DM_STP,
    },
};

static const struct behavior_parameter_value_metadata slot_param2_values[] = {
    {
        .display_name = "Recording Slot",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
        .range = {.min = 0, .max = CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_SLOTS - 1},
    },
};

static const struct behavior_parameter_metadata_set cmd_index_metadata_set = {
    .param1_values = cmd_param1_values,
    .param1_values_len = ARRAY_SIZE(cmd_param1_values),
    .param2_values_len = ARRAY_SIZE(slot_param2_values),
};

static const struct behavior_parameter_metadata_set metadata_sets[] = {cmd_index_metadata_set};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(metadata_sets),
    .sets = metadata_sets,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int behavior_dynamic_macro_init(const struct device *dev) {
    struct behavior_dynamic_macro_data *data = dev->data;
    memset(data->slots, 0,
           sizeof(struct behavior_dynamic_macro_slot) *
               CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_SLOTS);
    return 0;
};

static void behavior_dynamic_macro_start(const struct device *dev, const int dm_slot_index) {
    k_timer_stop(&dynamic_macro_work_timer);

    struct behavior_dynamic_macro_data *data = dev->data;
    data->slots[dm_slot_index].state = DYNAMIC_MACRO_STATE_RECORDING;
    data->slots[dm_slot_index].current_event = 0;
    data->slots[dm_slot_index].event_count = 0;
    LOG_DBG("Started recording dynamic macro %d", dm_slot_index);
};

static void behavior_dynamic_macro_play(const struct device *dev, const int dm_slot_index) {
    struct behavior_dynamic_macro_data *data = dev->data;

    if (data->slots[dm_slot_index].event_count == 0)
        return;

    LOG_DBG("Playing dynamic macro %d", dm_slot_index);
    data->slots[dm_slot_index].state = DYNAMIC_MACRO_STATE_PLAYING;
    data->slots[dm_slot_index].current_event = 0;
    k_timer_start(&dynamic_macro_work_timer, K_NO_WAIT,
                  K_MSEC(CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_TAP_DELAY));
};

static void behavior_dynamic_macro_stop(const struct device *dev, const int dm_slot_index) {
    struct behavior_dynamic_macro_data *data = dev->data;
    data->slots[dm_slot_index].state = DYNAMIC_MACRO_STATE_STOPPED;
    LOG_DBG("Stopped recording dynamic macro %d", dm_slot_index);
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    switch (binding->param1) {
    case DM_REC:
        behavior_dynamic_macro_start(dev, binding->param2);
        return ZMK_BEHAVIOR_OPAQUE;
    case DM_PLY:
        behavior_dynamic_macro_play(dev, binding->param2);
        return ZMK_BEHAVIOR_OPAQUE;
    case DM_STP:
        behavior_dynamic_macro_stop(dev, binding->param2);
        return ZMK_BEHAVIOR_OPAQUE;
    default:
        LOG_ERR("Unknown DM command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_dynamic_macro_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

void record_event(const int slot_index, struct behavior_dynamic_macro_slot *slot,
                  const struct zmk_keycode_state_changed *ev) {
    if (slot->event_count < CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_EVENTS) {
        LOG_DBG("Dynamic macro capturing event %d/%d for macro %d", slot->event_count,
                CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_EVENTS - 1, slot_index);

        slot->events[slot->event_count] = *ev;
        slot->event_count++;
    } else {
        LOG_WRN("Dynamic macro %d out of space. %d events were captured", slot_index,
                slot->event_count);
    }
}

int behavior_dynamic_macro_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    const struct device *dev = DEVICE_DT_INST_GET(0);

    if (!ev || !device_is_ready(dev))
        goto done;

    struct behavior_dynamic_macro_data *data = dev->data;

    for (int i = 0; i < CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_SLOTS; i++) {
        struct behavior_dynamic_macro_slot *slot = &data->slots[i];
        if (slot->state == DYNAMIC_MACRO_STATE_RECORDING) {
            record_event(i, slot, ev);
        }
    }

done:
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(DT_DRV_COMPAT, behavior_dynamic_macro_event_listener);
ZMK_SUBSCRIPTION(DT_DRV_COMPAT, zmk_keycode_state_changed);

BEHAVIOR_DT_INST_DEFINE(0, behavior_dynamic_macro_init, NULL, &dynamic_macro_data, NULL,
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_dynamic_macro_driver_api);

void dynamic_macro_work_handler(struct k_work *work) {
    const struct device *dev = DEVICE_DT_INST_GET(0);
    struct behavior_dynamic_macro_data *data = dev->data;

    int handled = 0;
    for (int i = 0; i < CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_SLOTS; i++) {
        struct behavior_dynamic_macro_slot *slot = &data->slots[i];

        if (slot->state != DYNAMIC_MACRO_STATE_PLAYING)
            continue;

        const uint32_t event = slot->current_event++;

        LOG_DBG("Sending dynamic macro event %d/%d", event, slot->event_count - 1);
        slot->events[event].timestamp = k_uptime_get();
        raise_zmk_keycode_state_changed(slot->events[event]);

        if (event + 1 >= slot->event_count ||
            event + 1 >= CONFIG_ZMK_BEHAVIOR_DYNAMIC_MACRO_MAX_EVENTS) {
            slot->current_event = 0;
            slot->state = DYNAMIC_MACRO_STATE_STOPPED;
        }

        handled++;
    }

    if (!handled)
        k_timer_stop(&dynamic_macro_work_timer);
}

K_WORK_DEFINE(dynamic_macro_work, dynamic_macro_work_handler);

void dynamic_macro_timer_submit(struct k_timer *_timer) { k_work_submit(&dynamic_macro_work); }
K_TIMER_DEFINE(dynamic_macro_work_timer, dynamic_macro_timer_submit, NULL);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
