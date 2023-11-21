/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "zmk/keys.h"
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/hid.h>
#include <dt-bindings/zmk/modifiers.h>

static struct zmk_hid_keyboard_report keyboard_report = {

    .report_id = ZMK_HID_REPORT_ID_KEYBOARD, .body = {.modifiers = 0, ._reserved = 0, .keys = {0}}};

static struct zmk_hid_consumer_report consumer_report = {.report_id = ZMK_HID_REPORT_ID_CONSUMER,
                                                         .body = {.keys = {0}}};

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)

static zmk_hid_boot_report_t boot_report = {.modifiers = 0, ._reserved = 0, .keys = {0}};
static uint8_t keys_held = 0;

#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

#if IS_ENABLED(CONFIG_ZMK_MOUSE)

static struct zmk_hid_mouse_report mouse_report = {
    .report_id = ZMK_HID_REPORT_ID_MOUSE,
    .body = {.buttons = 0, .d_x = 0, .d_y = 0, .d_scroll_y = 0}};

#endif // IS_ENABLED(CONFIG_ZMK_MOUSE)

// Keep track of how often a modifier was pressed.
// Only release the modifier if the count is 0.
static int explicit_modifier_counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static zmk_mod_flags_t explicit_modifiers = 0;
static zmk_mod_flags_t implicit_modifiers = 0;
static zmk_mod_flags_t masked_modifiers = 0;

#define SET_MODIFIERS(mods)                                                                        \
    {                                                                                              \
        keyboard_report.body.modifiers = (mods & ~masked_modifiers) | implicit_modifiers;          \
        LOG_DBG("Modifiers set to 0x%02X", keyboard_report.body.modifiers);                        \
    }

#define GET_MODIFIERS (keyboard_report.body.modifiers)

zmk_mod_flags_t zmk_hid_get_explicit_mods(void) { return explicit_modifiers; }

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

bool zmk_hid_mod_is_pressed(zmk_mod_t modifier) {
    zmk_mod_flags_t mod_flag = 1 << modifier;
    return (zmk_hid_get_explicit_mods() & mod_flag) == mod_flag;
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

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)

static zmk_hid_boot_report_t *boot_report_rollover(uint8_t modifiers) {
    boot_report.modifiers = modifiers;
    for (int i = 0; i < HID_BOOT_KEY_LEN; i++) {
        boot_report.keys[i] = HID_ERROR_ROLLOVER;
    }
    return &boot_report;
}

#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_NKRO)

#define TOGGLE_KEYBOARD(code, val) WRITE_BIT(keyboard_report.body.keys[code / 8], code % 8, val)

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
zmk_hid_boot_report_t *zmk_hid_get_boot_report(void) {
    if (keys_held > HID_BOOT_KEY_LEN) {
        return boot_report_rollover(keyboard_report.body.modifiers);
    }

    boot_report.modifiers = keyboard_report.body.modifiers;
    memset(&boot_report.keys, 0, HID_BOOT_KEY_LEN);
    int ix = 0;
    uint8_t base_code = 0;
    for (int i = 0; i < sizeof(keyboard_report.body.keys); ++i) {
        if (ix == keys_held) {
            break;
        }
        if (!keyboard_report.body.keys[i]) {
            continue;
        }
        base_code = i * 8;
        for (int j = 0; j < 8; ++j) {
            if (keyboard_report.body.keys[i] & BIT(j)) {
                boot_report.keys[ix++] = base_code + j;
            }
        }
    }
    return &boot_report;
}
#endif

static inline int select_keyboard_usage(zmk_key_t usage) {
    if (usage > ZMK_HID_KEYBOARD_NKRO_MAX_USAGE) {
        return -EINVAL;
    }
    TOGGLE_KEYBOARD(usage, 1);
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    ++keys_held;
#endif
    return 0;
}

static inline int deselect_keyboard_usage(zmk_key_t usage) {
    if (usage > ZMK_HID_KEYBOARD_NKRO_MAX_USAGE) {
        return -EINVAL;
    }
    TOGGLE_KEYBOARD(usage, 0);
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    --keys_held;
#endif
    return 0;
}

static inline bool check_keyboard_usage(zmk_key_t usage) {
    if (usage > ZMK_HID_KEYBOARD_NKRO_MAX_USAGE) {
        return false;
    }
    return keyboard_report.body.keys[usage / 8] & (1 << (usage % 8));
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

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
zmk_hid_boot_report_t *zmk_hid_get_boot_report(void) {
    if (keys_held > HID_BOOT_KEY_LEN) {
        return boot_report_rollover(keyboard_report.body.modifiers);
    }

#if CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE != HID_BOOT_KEY_LEN
    // Form a boot report from a report of different size.

    boot_report.modifiers = keyboard_report.body.modifiers;

    int out = 0;
    for (int i = 0; i < CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE; i++) {
        uint8_t key = keyboard_report.body.keys[i];
        if (key) {
            boot_report.keys[out++] = key;
            if (out == keys_held) {
                break;
            }
        }
    }

    while (out < HID_BOOT_KEY_LEN) {
        boot_report.keys[out++] = 0;
    }

    return &boot_report;
#else
    return &keyboard_report.body;
#endif /* CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE != HID_BOOT_KEY_LEN */
}
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

static inline int select_keyboard_usage(zmk_key_t usage) {
    TOGGLE_KEYBOARD(0U, usage);
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    ++keys_held;
#endif
    return 0;
}

static inline int deselect_keyboard_usage(zmk_key_t usage) {
    TOGGLE_KEYBOARD(usage, 0U);
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    --keys_held;
#endif
    return 0;
}

static inline int check_keyboard_usage(zmk_key_t usage) {
    for (int idx = 0; idx < CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE; idx++) {
        if (keyboard_report.body.keys[idx] == usage) {
            return true;
        }
    }
    return false;
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

int zmk_hid_implicit_modifiers_release(void) {
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

int zmk_hid_masked_modifiers_clear(void) {
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

bool zmk_hid_keyboard_is_pressed(zmk_key_t code) {
    if (code >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL && code <= HID_USAGE_KEY_KEYBOARD_RIGHT_GUI) {
        return zmk_hid_mod_is_pressed(code - HID_USAGE_KEY_KEYBOARD_LEFTCONTROL);
    }
    return check_keyboard_usage(code);
}

void zmk_hid_keyboard_clear(void) {
    memset(&keyboard_report.body, 0, sizeof(keyboard_report.body));
}

int zmk_hid_consumer_press(zmk_key_t code) {
    TOGGLE_CONSUMER(0U, code);
    return 0;
};

int zmk_hid_consumer_release(zmk_key_t code) {
    TOGGLE_CONSUMER(code, 0U);
    return 0;
};

void zmk_hid_consumer_clear(void) {
    memset(&consumer_report.body, 0, sizeof(consumer_report.body));
}

bool zmk_hid_consumer_is_pressed(zmk_key_t key) {
    for (int idx = 0; idx < CONFIG_ZMK_HID_CONSUMER_REPORT_SIZE; idx++) {
        if (consumer_report.body.keys[idx] == key) {
            return true;
        }
    }
    return false;
}

int zmk_hid_press(uint32_t usage) {
    switch (ZMK_HID_USAGE_PAGE(usage)) {
    case HID_USAGE_KEY:
        return zmk_hid_keyboard_press(ZMK_HID_USAGE_ID(usage));
    case HID_USAGE_CONSUMER:
        return zmk_hid_consumer_press(ZMK_HID_USAGE_ID(usage));
    }
    return -EINVAL;
}

int zmk_hid_release(uint32_t usage) {
    switch (ZMK_HID_USAGE_PAGE(usage)) {
    case HID_USAGE_KEY:
        return zmk_hid_keyboard_release(ZMK_HID_USAGE_ID(usage));
    case HID_USAGE_CONSUMER:
        return zmk_hid_consumer_release(ZMK_HID_USAGE_ID(usage));
    }
    return -EINVAL;
}

bool zmk_hid_is_pressed(uint32_t usage) {
    switch (ZMK_HID_USAGE_PAGE(usage)) {
    case HID_USAGE_KEY:
        return zmk_hid_keyboard_is_pressed(ZMK_HID_USAGE_ID(usage));
    case HID_USAGE_CONSUMER:
        return zmk_hid_consumer_is_pressed(ZMK_HID_USAGE_ID(usage));
    }
    return false;
}

#if IS_ENABLED(CONFIG_ZMK_MOUSE)

// Keep track of how often a button was pressed.
// Only release the button if the count is 0.
static int explicit_button_counts[5] = {0, 0, 0, 0, 0};
static zmk_mod_flags_t explicit_buttons = 0;

#define SET_MOUSE_BUTTONS(btns)                                                                    \
    {                                                                                              \
        mouse_report.body.buttons = btns;                                                          \
        LOG_DBG("Mouse buttons set to 0x%02X", mouse_report.body.buttons);                         \
    }

int zmk_hid_mouse_button_press(zmk_mouse_button_t button) {
    if (button >= ZMK_HID_MOUSE_NUM_BUTTONS) {
        return -EINVAL;
    }

    explicit_button_counts[button]++;
    LOG_DBG("Button %d count %d", button, explicit_button_counts[button]);
    WRITE_BIT(explicit_buttons, button, true);
    SET_MOUSE_BUTTONS(explicit_buttons);
    return 0;
}

int zmk_hid_mouse_button_release(zmk_mouse_button_t button) {
    if (button >= ZMK_HID_MOUSE_NUM_BUTTONS) {
        return -EINVAL;
    }

    if (explicit_button_counts[button] <= 0) {
        LOG_ERR("Tried to release button %d too often", button);
        return -EINVAL;
    }
    explicit_button_counts[button]--;
    LOG_DBG("Button %d count: %d", button, explicit_button_counts[button]);
    if (explicit_button_counts[button] == 0) {
        LOG_DBG("Button %d released", button);
        WRITE_BIT(explicit_buttons, button, false);
    }
    SET_MOUSE_BUTTONS(explicit_buttons);
    return 0;
}

int zmk_hid_mouse_buttons_press(zmk_mouse_button_flags_t buttons) {
    for (zmk_mouse_button_t i = 0; i < ZMK_HID_MOUSE_NUM_BUTTONS; i++) {
        if (buttons & BIT(i)) {
            zmk_hid_mouse_button_press(i);
        }
    }
    return 0;
}

int zmk_hid_mouse_buttons_release(zmk_mouse_button_flags_t buttons) {
    for (zmk_mouse_button_t i = 0; i < ZMK_HID_MOUSE_NUM_BUTTONS; i++) {
        if (buttons & BIT(i)) {
            zmk_hid_mouse_button_release(i);
        }
    }
    return 0;
}

void zmk_hid_mouse_movement_set(int16_t x, int16_t y) {
    mouse_report.body.d_x = x;
    mouse_report.body.d_y = y;
    LOG_DBG("Mouse movement set to %d/%d", mouse_report.body.d_x, mouse_report.body.d_y);
}

void zmk_hid_mouse_movement_update(int16_t x, int16_t y) {
    mouse_report.body.d_x += x;
    mouse_report.body.d_y += y;
    LOG_DBG("Mouse movement updated to %d/%d", mouse_report.body.d_x, mouse_report.body.d_y);
}

void zmk_hid_mouse_scroll_set(int8_t x, int8_t y) {
    mouse_report.body.d_scroll_x = x;
    mouse_report.body.d_scroll_y = y;
    LOG_DBG("Mouse scroll set to %d/%d", mouse_report.body.d_scroll_x,
            mouse_report.body.d_scroll_y);
}

void zmk_hid_mouse_scroll_update(int8_t x, int8_t y) {
    mouse_report.body.d_scroll_x += x;
    mouse_report.body.d_scroll_y += y;
    LOG_DBG("Mouse scroll updated to X: %d/%d", mouse_report.body.d_scroll_x,
            mouse_report.body.d_scroll_y);
}

void zmk_hid_mouse_clear(void) {
    LOG_DBG("Mouse report cleared");
    memset(&mouse_report.body, 0, sizeof(mouse_report.body));
}

#endif // IS_ENABLED(CONFIG_ZMK_MOUSE)

struct zmk_hid_keyboard_report *zmk_hid_get_keyboard_report(void) {
    return &keyboard_report;
}

struct zmk_hid_consumer_report *zmk_hid_get_consumer_report(void) {
    return &consumer_report;
}

#if IS_ENABLED(CONFIG_ZMK_MOUSE)

struct zmk_hid_mouse_report *zmk_hid_get_mouse_report(void) {
    return &mouse_report;
}

#endif // IS_ENABLED(CONFIG_ZMK_MOUSE)
