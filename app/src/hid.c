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

static struct zmk_hid_mouse_report mouse_report = {.report_id = 3, .body = {
    .buttons = 0, .x = 0, .y = 0}};

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


// Keep track of how often a button was pressed.
// Only release the button if the count is 0.
static int explicit_button_counts[3] = {0, 0, 0};
static zmk_mod_flags_t explicit_buttons = 0;

#define SET_MOUSE_BUTTONS(butts)                                                               \
    {                                                                                            \
        mouse_report.body.buttons = butts;                                                     \
        LOG_DBG("Mouse buttons set to 0x%02X", mouse_report.body.buttons);                       \
    }

int zmk_hid_mouse_button_press(zmk_mouse_button_t button) {
    explicit_button_counts[button-5]++;
    LOG_DBG("Button %d count %d", button, explicit_button_counts[button-5]);
    WRITE_BIT(explicit_buttons, button, true);
    SET_MOUSE_BUTTONS(explicit_buttons);
    return 0;
}

int zmk_hid_mouse_button_release(zmk_mouse_button_t button) {
    if (explicit_button_counts[button-5] <= 0) {
        LOG_ERR("Tried to release button %d too often", button);
        return -EINVAL;
    }
    explicit_button_counts[button]--;
    LOG_DBG("Button %d count: %d", button, explicit_button_counts[button-5]);
    if (explicit_button_counts[button-5] == 0) {
        LOG_DBG("Button %d released", button);
        WRITE_BIT(explicit_buttons, button, false);
    }
    SET_MOUSE_BUTTONS(explicit_buttons);
    return 0;
}

int zmk_hid_mouse_buttons_press(zmk_mouse_button_flags_t buttons) {
    for (zmk_mod_t i = 5; i < 8; i++) {
        if (buttons & (1 << i)) {
            zmk_hid_mouse_button_press(i);
        }
    }
    return 0;
}

int zmk_hid_mouse_buttons_release(zmk_mouse_button_flags_t buttons) {
    for (zmk_mod_t i = 5; i < 8; i++) {
        if (buttons & (1 << i)) {
            zmk_hid_mouse_button_release(i);
        }
    }
    return 0;
}

void zmk_hid_mouse_clear() { memset(&mouse_report.body, 0, sizeof(mouse_report.body)); }

struct zmk_hid_keyboard_report *zmk_hid_get_keyboard_report() {
    return &keyboard_report;
}

struct zmk_hid_consumer_report *zmk_hid_get_consumer_report() {
    return &consumer_report;
}

struct zmk_hid_mouse_report *zmk_hid_get_mouse_report() {
    return &mouse_report;
}
