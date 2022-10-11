/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_capslock

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>

#include <zmk/behavior_queue.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keys.h>
#include <zmk/keymap.h>
#include <zmk/led_indicators.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct capslock_key_item {
    uint16_t page;
    uint32_t id;
    uint8_t implicit_modifiers;
};

struct behavior_capslock_config {
    uint8_t index;
    struct zmk_behavior_binding capslock_binding;
    uint32_t capslock_binding_hold_time;
    bool enable_on_press;
    bool disable_on_release;
    bool disable_on_next_release;
    uint8_t disable_on_keys_count;
    struct capslock_key_item disable_on_keys[];
};

struct behavior_capslock_data {
    uint32_t position;
    bool active;
    bool just_activated;
};

static void toggle_capslock(const struct device *dev) {
    const struct behavior_capslock_config *config = dev->config;
    const struct behavior_capslock_data *data = dev->data;

    zmk_behavior_queue_add(data->position, config->capslock_binding, true,
                           config->capslock_binding_hold_time);
    zmk_behavior_queue_add(data->position, config->capslock_binding, false, 0);
}

static void set_capslock_state(const struct device *dev, const bool target_state) {
    const bool current_state =
        zmk_led_indicators_get_current_flags() & ZMK_LED_INDICATORS_CAPSLOCK_BIT;

    if (current_state != target_state) {
        LOG_DBG("toggling capslock state from %d to %d", current_state, target_state);
        toggle_capslock(dev);
    } else {
        LOG_DBG("capslock state is already %d", target_state);
    }
}

static void activate_capslock(const struct device *dev) {
    struct behavior_capslock_data *data = dev->data;

    set_capslock_state(dev, true);

    if (!data->active) {
        data->just_activated = true;
    }
    data->active = true;
}

static void deactivate_capslock(const struct device *dev) {
    struct behavior_capslock_data *data = dev->data;

    set_capslock_state(dev, false);
    data->active = false;
}

static int on_capslock_binding_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_capslock_config *config = dev->config;
    struct behavior_capslock_data *data = dev->data;

    data->position = event.position;
    if (config->enable_on_press) {
        LOG_DBG("activating capslock (enable-on-press)");
        activate_capslock(dev);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_capslock_binding_released(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_capslock_config *config = dev->config;
    struct behavior_capslock_data *data = dev->data;

    if (config->disable_on_release) {
        LOG_DBG("deactivating capslock (disable-on-release)");
        deactivate_capslock(dev);
    } else if (config->disable_on_next_release && !data->just_activated) {
        LOG_DBG("deactivating capslock (disable-on-next-release)");
        deactivate_capslock(dev);
    }

    data->just_activated = false;

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_capslock_driver_api = {
    .binding_pressed = on_capslock_binding_pressed,
    .binding_released = on_capslock_binding_released,
};

static int capslock_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_capslock, capslock_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_capslock, zmk_keycode_state_changed);

static const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

static bool capslock_find_key_item(const struct capslock_key_item *key_items,
                                   const size_t key_items_count, uint16_t usage_page,
                                   uint8_t usage_id, uint8_t implicit_modifiers) {
    for (int i = 0; i < key_items_count; i++) {
        const struct capslock_key_item *break_item = &key_items[i];
        LOG_DBG("Comparing with 0x%02X - 0x%02X (with implicit mods: 0x%02X)", break_item->page,
                break_item->id, break_item->implicit_modifiers);

        if (break_item->page == usage_page && break_item->id == usage_id &&
            (break_item->implicit_modifiers & (implicit_modifiers | zmk_hid_get_explicit_mods())) ==
                break_item->implicit_modifiers) {
            LOG_DBG("found included usage: 0x%02X - 0x%02X", usage_page, usage_id);
            return true;
        }
    }

    return false;
}

static int capslock_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); i++) {
        const struct device *dev = devs[i];
        if (dev == NULL) {
            continue;
        }

        struct behavior_capslock_data *data = dev->data;
        if (!data->active) {
            continue;
        }

        const struct behavior_capslock_config *config = dev->config;

        if (capslock_find_key_item(config->disable_on_keys, config->disable_on_keys_count,
                                   ev->usage_page, ev->keycode, ev->implicit_modifiers)) {
            LOG_DBG("deactivating capslock (disable-on-keys: 0x%02X-0x%02X)", ev->usage_page,
                    ev->keycode);
            deactivate_capslock(dev);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int behavior_capslock_init(const struct device *dev) {
    const struct behavior_capslock_config *config = dev->config;
    devs[config->index] = dev;
    return 0;
}

#define PARSE_KEY_ITEM(i)                                                                          \
    {.page = ZMK_HID_USAGE_PAGE(i),                                                                \
     .id = ZMK_HID_USAGE_ID(i),                                                                    \
     .implicit_modifiers = SELECT_MODS(i)},

#define BREAK_ITEM(i, n) PARSE_KEY_ITEM(DT_INST_PROP_BY_IDX(n, disable_on_keys, i))

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    }

#define KP_INST(n)                                                                                 \
    static struct behavior_capslock_data behavior_capslock_data_##n = {.active = false};           \
    static struct behavior_capslock_config behavior_capslock_config_##n = {                        \
        .index = n,                                                                                \
        .capslock_binding = _TRANSFORM_ENTRY(0, n),                                                \
        .capslock_binding_hold_time = DT_INST_PROP(n, binding_hold_time),                          \
        .enable_on_press = DT_INST_PROP(n, enable_on_press),                                       \
        .disable_on_release = DT_INST_PROP(n, disable_on_release),                                 \
        .disable_on_next_release = DT_INST_PROP(n, disable_on_next_release),                       \
        .disable_on_keys = {UTIL_LISTIFY(DT_INST_PROP_LEN(n, disable_on_keys), BREAK_ITEM, n)},    \
        .disable_on_keys_count = DT_INST_PROP_LEN(n, disable_on_keys),                             \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, behavior_capslock_init, NULL, &behavior_capslock_data_##n,            \
                          &behavior_capslock_config_##n, APPLICATION,                              \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_capslock_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
