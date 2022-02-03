/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/hid.h>
#include <dt-bindings/zmk/modifiers.h>

static struct zmk_hid_keyboard_report keyboard_report = {
    .report_id = 1, .body = {.modifiers = 0, ._reserved = 0, .keys = {0}}};

static struct zmk_hid_consumer_report consumer_report = {.report_id = 2, .body = {.keys = {0}}};

// Keep track of how often a modifier was pressed.
// Only release the modifier if the count is 0.
static int explicit_modifier_counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static zmk_mod_flags_t explicit_modifiers = 0;
static zmk_mod_flags_t implicit_modifiers = 0;
static zmk_mod_flags_t masked_modifiers = 0;

#define SET_MODIFIERS(mods)                                                                        \
    {                                                                                              \
        keyboard_report.body.modifiers = (mods | implicit_modifiers) & ~masked_modifiers;          \
        LOG_DBG("Modifiers set to 0x%02X", keyboard_report.body.modifiers);                        \
    }

#define GET_MODIFIERS (keyboard_report.body.modifiers)

zmk_mod_flags_t zmk_hid_get_explicit_mods() { return explicit_modifiers; }

int zmk_hid_register_mod(zmk_mod_t modifier) {
    explicit_modifier_counts[modifier]++;
    LOG_DBG("Modifier %d count %d", modifier, explicit_modifier_counts[modifier]);
    WRITE_BIT(explicit_modifiers, modifier, true);
    zmk_mod_flags_t current = GET_MODIFIERS;
    SET_MODIFIERS(explicit_modifiers);
    return current == GET_MODIFIERS ? 0 : 1;
}

int zmk_hid_unregister_mod(zmk_mod_t modifier) {
    if (explicit_modifier_counts[modifier] <= 0) {
        LOG_ERR("Tried to unregister modifier %d too often", modifier);
        return -EINVAL;
    }
    explicit_modifier_counts[modifier]--;
    LOG_DBG("Modifier %d count: %d", modifier, explicit_modifier_counts[modifier]);
    if (explicit_modifier_counts[modifier] == 0) {
        LOG_DBG("Modifier %d released", modifier);
        WRITE_BIT(explicit_modifiers, modifier, false);
    }
    zmk_mod_flags_t current = GET_MODIFIERS;
    SET_MODIFIERS(explicit_modifiers);
    return current == GET_MODIFIERS ? 0 : 1;
}

int zmk_hid_register_mods(zmk_mod_flags_t modifiers) {
    int ret = 0;
    for (zmk_mod_t i = 0; i < 8; i++) {
        if (modifiers & (1 << i)) {
            ret += zmk_hid_register_mod(i);
        }
    }
    return ret;
}

int zmk_hid_unregister_mods(zmk_mod_flags_t modifiers) {
    int ret = 0;
    for (zmk_mod_t i = 0; i < 8; i++) {
        if (modifiers & (1 << i)) {
            ret += zmk_hid_unregister_mod(i);
        }
    }

    return ret;
}

#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_NKRO)

#define TOGGLE_KEYBOARD(code, val) WRITE_BIT(keyboard_report.body.keys[code / 8], code % 8, val)

static inline int select_keyboard_usage(zmk_key_t usage) {
    if (usage > ZMK_HID_KEYBOARD_NKRO_MAX_USAGE) {
        return -EINVAL;
    }
    TOGGLE_KEYBOARD(usage, 1);
    return 0;
}

static inline int deselect_keyboard_usage(zmk_key_t usage) {
    if (usage > ZMK_HID_KEYBOARD_NKRO_MAX_USAGE) {
        return -EINVAL;
    }
    TOGGLE_KEYBOARD(usage, 0);
    return 0;
}

#elif IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_HKRO)

#define TOGGLE_KEYBOARD(match, val)                                                                \
    for (int idx = 0; idx < CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE; idx++) {                          \
        if (keyboard_report.body.keys[idx] != match) {                                             \
            continue;                                                                              \
        }                                                                                          \
        keyboard_report.body.keys[idx] = val;                                                      \
        if (val) {                                                                                 \
            break;                                                                                 \
        }                                                                                          \
    }

static inline int select_keyboard_usage(zmk_key_t usage) {
    TOGGLE_KEYBOARD(0U, usage);
    return 0;
}

static inline int deselect_keyboard_usage(zmk_key_t usage) {
    TOGGLE_KEYBOARD(usage, 0U);
    return 0;
}

#else
#error "A proper HID report type must be selected"
#endif

#define TOGGLE_CONSUMER(match, val)                                                                \
    COND_CODE_1(IS_ENABLED(CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_BASIC),                           \
                (if (val > 0xFF) { return -ENOTSUP; }), ())                                        \
    for (int idx = 0; idx < CONFIG_ZMK_HID_CONSUMER_REPORT_SIZE; idx++) {                          \
        if (consumer_report.body.keys[idx] != match) {                                             \
            continue;                                                                              \
        }                                                                                          \
        consumer_report.body.keys[idx] = val;                                                      \
        if (val) {                                                                                 \
            break;                                                                                 \
        }                                                                                          \
    }

int zmk_hid_implicit_modifiers_press(zmk_mod_flags_t new_implicit_modifiers) {
    implicit_modifiers = new_implicit_modifiers;
    zmk_mod_flags_t current = GET_MODIFIERS;
    SET_MODIFIERS(explicit_modifiers);
    return current == GET_MODIFIERS ? 0 : 1;
}

int zmk_hid_implicit_modifiers_release() {
    implicit_modifiers = 0;
    zmk_mod_flags_t current = GET_MODIFIERS;
    SET_MODIFIERS(explicit_modifiers);
    return current == GET_MODIFIERS ? 0 : 1;
}

int zmk_hid_masked_modifiers_set(zmk_mod_flags_t new_masked_modifiers) {
    masked_modifiers = new_masked_modifiers;
    zmk_mod_flags_t current = GET_MODIFIERS;
    SET_MODIFIERS(explicit_modifiers);
    return current == GET_MODIFIERS ? 0 : 1;
}

int zmk_hid_masked_modifiers_clear() {
    masked_modifiers = 0;
    zmk_mod_flags_t current = GET_MODIFIERS;
    SET_MODIFIERS(explicit_modifiers);
    return current == GET_MODIFIERS ? 0 : 1;
}

int zmk_hid_keyboard_press(zmk_key_t code) {
    if (code >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL && code <= HID_USAGE_KEY_KEYBOARD_RIGHT_GUI) {
        return zmk_hid_register_mod(code - HID_USAGE_KEY_KEYBOARD_LEFTCONTROL);
    }
    select_keyboard_usage(code);
    return 0;
};

int zmk_hid_keyboard_release(zmk_key_t code) {
    if (code >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL && code <= HID_USAGE_KEY_KEYBOARD_RIGHT_GUI) {
        return zmk_hid_unregister_mod(code - HID_USAGE_KEY_KEYBOARD_LEFTCONTROL);
    }
    deselect_keyboard_usage(code);
    return 0;
};

void zmk_hid_keyboard_clear() { memset(&keyboard_report.body, 0, sizeof(keyboard_report.body)); }

int zmk_hid_consumer_press(zmk_key_t code) {
    TOGGLE_CONSUMER(0U, code);
    return 0;
};

int zmk_hid_consumer_release(zmk_key_t code) {
    TOGGLE_CONSUMER(code, 0U);
    return 0;
};

void zmk_hid_consumer_clear() { memset(&consumer_report.body, 0, sizeof(consumer_report.body)); }

struct zmk_hid_keyboard_report *zmk_hid_get_keyboard_report() {
    return &keyboard_report;
}

struct zmk_hid_consumer_report *zmk_hid_get_consumer_report() {
    return &consumer_report;
}
