/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_caps_word

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

struct caps_word_break {
    uint16_t page;
    uint32_t id;
};

struct behavior_caps_word_config {
    zmk_mod_flags_t mods;
    uint8_t index;
    uint8_t breaks_count;
    struct caps_word_break breaks[];
};

struct behavior_caps_word_data {
    bool active;
};

static void activate_caps_word(const struct device *dev) {
    struct behavior_caps_word_data *data = dev->data;
    const struct behavior_caps_word_config *config = dev->config;

    LOG_DBG("Registering mods 0x%02X from caps word device %s", config->mods,
            log_strdup(dev->name));
    zmk_hid_register_mods(config->mods);
    data->active = true;
}

static void deactivate_caps_word(const struct device *dev) {
    struct behavior_caps_word_data *data = dev->data;
    const struct behavior_caps_word_config *config = dev->config;

    LOG_DBG("Unregistering mods 0x%02X from caps word device %s", config->mods,
            log_strdup(dev->name));
    zmk_hid_unregister_mods(config->mods);
    data->active = false;
}

static int on_caps_word_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
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

static const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

static int caps_word_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); i++) {
        const struct device *dev = devs[i];
        if (dev == NULL) {
            continue;
        }

        struct behavior_caps_word_data *data = dev->data;
        if (!data->active) {
            continue;
        }

        const struct behavior_caps_word_config *config = dev->config;
        for (int b = 0; b < config->breaks_count; b++) {
            const struct caps_word_break *break_val = &config->breaks[b];

            if (break_val->page == ev->usage_page && break_val->id == ev->keycode) {
                deactivate_caps_word(dev);
            }
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int behavior_caps_word_init(const struct device *dev) {
    const struct behavior_caps_word_config *config = dev->config;
    devs[config->index] = dev;
    return 0;
}

#define CAPS_WORD_LABEL(i, _n) DT_INST_LABEL(i)

#define PARSE_BREAK(b) {.page = HID_USAGE_PAGE(b), .id = HID_USAGE_ID(b)},

#define BREAK_ITEM(i, n) PARSE_BREAK(DT_INST_PROP_BY_IDX(n, breaks, i))

#define KP_INST(n)                                                                                 \
    static struct behavior_caps_word_data behavior_caps_word_data_##n = {.active = false};         \
    static struct behavior_caps_word_config behavior_caps_word_config_##n = {                      \
        .index = n,                                                                                \
        .mods = DT_INST_PROP_OR(n, mods, MOD_LSFT),                                                \
        .breaks = {UTIL_LISTIFY(DT_INST_PROP_LEN(n, breaks), BREAK_ITEM, n)},                      \
        .breaks_count = DT_INST_PROP_LEN(n, breaks),                                               \
    };                                                                                             \
    DEVICE_AND_API_INIT(behavior_caps_word_##n, DT_INST_LABEL(n), behavior_caps_word_init,         \
                        &behavior_caps_word_data_##n, &behavior_caps_word_config_##n, APPLICATION, \
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_caps_word_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
