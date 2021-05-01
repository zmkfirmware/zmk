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

#define SET_MODIFIERS(mods)                                                                        \
    {                                                                                              \
        keyboard_report.body.modifiers = mods;                                                     \
        LOG_DBG("Modifiers set to 0x%02X", keyboard_report.body.modifiers);                        \
    }

zmk_mod_flags_t zmk_hid_get_explicit_mods() { return explicit_modifiers; }

int zmk_hid_register_mod(zmk_mod_t modifier) {
    explicit_modifier_counts[modifier]++;
    LOG_DBG("Modifier %d count %d", modifier, explicit_modifier_counts[modifier]);
    WRITE_BIT(explicit_modifiers, modifier, true);
    SET_MODIFIERS(explicit_modifiers);
    return 0;
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
    SET_MODIFIERS(explicit_modifiers);
    return 0;
}

int zmk_hid_register_mods(zmk_mod_flags_t modifiers) {
    for (zmk_mod_t i = 0; i < 8; i++) {
        if (modifiers & (1 << i)) {
            zmk_hid_register_mod(i);
        }
    }
    return 0;
}

int zmk_hid_unregister_mods(zmk_mod_flags_t modifiers) {
    for (zmk_mod_t i = 0; i < 8; i++) {
        if (modifiers & (1 << i)) {
            zmk_hid_unregister_mod(i);
        }
    }
    return 0;
}

#define TOGGLE_KEYBOARD(match, val)                                                                \
    for (int idx = 0; idx < ZMK_HID_KEYBOARD_NKRO_SIZE; idx++) {                                   \
        if (keyboard_report.body.keys[idx] != match) {                                             \
            continue;                                                                              \
        }                                                                                          \
        keyboard_report.body.keys[idx] = val;                                                      \
        if (val) {                                                                                 \
            break;                                                                                 \
        }                                                                                          \
    }

#define TOGGLE_CONSUMER(match, val)                                                                \
    for (int idx = 0; idx < ZMK_HID_CONSUMER_NKRO_SIZE; idx++) {                                   \
        if (consumer_report.body.keys[idx] != match) {                                             \
            continue;                                                                              \
        }                                                                                          \
        consumer_report.body.keys[idx] = val;                                                      \
        if (val) {                                                                                 \
            break;                                                                                 \
        }                                                                                          \
    }

int zmk_hid_implicit_modifiers_press(zmk_mod_flags_t implicit_modifiers) {
    SET_MODIFIERS(explicit_modifiers | implicit_modifiers);
    return 0;
}

int zmk_hid_implicit_modifiers_release() {
    SET_MODIFIERS(explicit_modifiers);
    return 0;
}

int zmk_hid_keyboard_press(zmk_key_t code) {
    if (code >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL && code <= HID_USAGE_KEY_KEYBOARD_RIGHT_GUI) {
        return zmk_hid_register_mod(code - HID_USAGE_KEY_KEYBOARD_LEFTCONTROL);
    }
    TOGGLE_KEYBOARD(0U, code);
    return 0;
};

int zmk_hid_keyboard_release(zmk_key_t code) {
    if (code >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL && code <= HID_USAGE_KEY_KEYBOARD_RIGHT_GUI) {
        return zmk_hid_unregister_mod(code - HID_USAGE_KEY_KEYBOARD_LEFTCONTROL);
    }
    TOGGLE_KEYBOARD(code, 0U);
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
