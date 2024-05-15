/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_caps_word

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/keys.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct key_list {
    size_t size;
    struct zmk_key_param keys[];
};

struct behavior_caps_word_config {
    const struct key_list *continue_keys;
    const struct key_list *shift_keys;
    zmk_mod_flags_t mods;
    bool no_default_keys;
};

struct behavior_caps_word_data {
    bool active;
};

static void activate_caps_word(const struct device *dev) {
    struct behavior_caps_word_data *data = dev->data;

    data->active = true;
}

static void deactivate_caps_word(const struct device *dev) {
    struct behavior_caps_word_data *data = dev->data;

    data->active = false;
}

static int on_caps_word_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_caps_word_data *data = dev->data;

    if (data->active) {
        deactivate_caps_word(dev);
    } else {
        activate_caps_word(dev);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_caps_word_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_caps_word_driver_api = {
    .binding_pressed = on_caps_word_binding_pressed,
    .binding_released = on_caps_word_binding_released,
};

static int caps_word_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_caps_word, caps_word_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_caps_word, zmk_keycode_state_changed);

#define DEVICE_COUNT DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)
#define DEVICE_INST(n, _) DEVICE_DT_GET(DT_DRV_INST(n))

static const struct device *devs[] = {LISTIFY(DEVICE_COUNT, DEVICE_INST, (, ))};

static bool key_list_contains(const struct key_list *list, uint16_t usage_page, zmk_key_t usage_id,
                              zmk_mod_flags_t modifiers) {
    for (int i = 0; i < list->size; i++) {
        const struct zmk_key_param *key = &list->keys[i];

        if (key->page == usage_page && key->id == usage_id &&
            (key->modifiers & modifiers) == key->modifiers) {
            return true;
        }
    }

    return false;
}

static bool caps_word_is_alpha(uint16_t usage_page, zmk_key_t usage_id) {
    if (usage_page != HID_USAGE_KEY) {
        return false;
    }

    return usage_id >= HID_USAGE_KEY_KEYBOARD_A && usage_id <= HID_USAGE_KEY_KEYBOARD_Z;
}

static bool caps_word_is_numeric(uint16_t usage_page, zmk_key_t usage_id) {
    if (usage_page != HID_USAGE_KEY) {
        return false;
    }

    return ((usage_id >= HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION &&
             usage_id <= HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS) ||
            (usage_id >= HID_USAGE_KEY_KEYPAD_1_AND_END &&
             usage_id <= HID_USAGE_KEY_KEYPAD_0_AND_INSERT)) ||
           usage_id == HID_USAGE_KEY_KEYPAD_00 || usage_id == HID_USAGE_KEY_KEYPAD_000;
}

static bool caps_word_should_enhance(const struct behavior_caps_word_config *config,
                                     struct zmk_keycode_state_changed *ev) {
    // Unless no-default-keys is set, alpha keys are enhanced.
    if (!config->no_default_keys && caps_word_is_alpha(ev->usage_page, ev->keycode)) {
        return true;
    }

    zmk_mod_flags_t modifiers = ev->implicit_modifiers | zmk_hid_get_explicit_mods();

    return key_list_contains(config->shift_keys, ev->usage_page, ev->keycode, modifiers);
}

static bool caps_word_should_continue(const struct behavior_caps_word_config *config,
                                      struct zmk_keycode_state_changed *ev) {
    // Modifiers do not break a word, nor does any key that is enhanced.
    if (is_mod(ev->usage_page, ev->keycode) || caps_word_should_enhance(config, ev)) {
        return true;
    }

    // Unless no-default-keys is set, number keys do not break a word.
    if (!config->no_default_keys && caps_word_is_numeric(ev->usage_page, ev->keycode)) {
        return true;
    }

    zmk_mod_flags_t modifiers = ev->implicit_modifiers | zmk_hid_get_explicit_mods();

    return key_list_contains(config->continue_keys, ev->usage_page, ev->keycode, modifiers);
}

static int caps_word_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < ARRAY_SIZE(devs); i++) {
        const struct device *dev = devs[i];
        const struct behavior_caps_word_data *data = dev->data;
        if (!data->active) {
            continue;
        }

        const struct behavior_caps_word_config *config = dev->config;

        if (caps_word_should_enhance(config, ev)) {
            LOG_DBG("Enhancing usage 0x%02X with modifiers: 0x%02X", ev->keycode, config->mods);
            ev->implicit_modifiers |= config->mods;
        }

        if (!caps_word_should_continue(config, ev)) {
            LOG_DBG("Deactivating caps_word for 0x%02X - 0x%02X", ev->usage_page, ev->keycode);
            deactivate_caps_word(dev);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int behavior_caps_word_init(const struct device *dev) { return 0; }

#define KEY_LIST_ITEM(i, n, prop) ZMK_KEY_PARAM_DECODE(DT_INST_PROP_BY_IDX(n, prop, i))

#define PROP_KEY_LIST(n, prop)                                                                     \
    COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), prop),                                            \
                ({                                                                                 \
                    .size = DT_INST_PROP_LEN(n, prop),                                             \
                    .keys = {LISTIFY(DT_INST_PROP_LEN(n, prop), KEY_LIST_ITEM, (, ), n, prop)},    \
                }),                                                                                \
                ({.size = 0}))

#define KP_INST(n)                                                                                 \
    static const struct key_list caps_word_continue_list_##n = PROP_KEY_LIST(n, continue_list);    \
    static const struct key_list caps_word_shift_list_##n = PROP_KEY_LIST(n, shift_list);          \
                                                                                                   \
    static struct behavior_caps_word_data behavior_caps_word_data_##n = {.active = false};         \
    static struct behavior_caps_word_config behavior_caps_word_config_##n = {                      \
        .mods = DT_INST_PROP_OR(n, mods, MOD_LSFT),                                                \
        .continue_keys = &caps_word_continue_list_##n,                                             \
        .shift_keys = &caps_word_shift_list_##n,                                                   \
        .no_default_keys = DT_INST_PROP(n, no_default_keys),                                       \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_caps_word_init, NULL, &behavior_caps_word_data_##n,        \
                            &behavior_caps_word_config_##n, POST_KERNEL,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_caps_word_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)
