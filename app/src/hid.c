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
    .report_id = HID_REPORT_ID_KEYBOARD, .body = {.modifiers = 0, ._reserved = 0, .keys = {0}}};

static struct zmk_hid_consumer_report consumer_report = {.report_id = HID_REPORT_ID_CONSUMER, .body = {.keys = {0}}};

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
#if !IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_HKRO) || CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE != HID_BOOT_KEY_LEN
static zmk_hid_boot_report_t boot_report = {
    .modifiers = 0,
    ._reserved = 0,
    .keys = {0}
};
#endif
static uint8_t keys_held = 0;
#endif

// Keep track of how often a modifier was pressed.
// Only release the modifier if the count is 0.
static int explicit_modifier_counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static zmk_mod_flags_t explicit_modifiers = 0;

#define SET_MODIFIERS(mods)                                                                        \
    {                                                                                              \
        keyboard_report.body.modifiers = mods;                                                     \
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

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)

static zmk_hid_boot_report_t error_report = {
    .modifiers = 0,
    ._reserved = 0,
    .keys = {
        HID_ERROR_ROLLOVER, HID_ERROR_ROLLOVER, HID_ERROR_ROLLOVER,
        HID_ERROR_ROLLOVER, HID_ERROR_ROLLOVER, HID_ERROR_ROLLOVER
    }
};

#define HID_CHECK_ROLLOVER_ERROR() \
    if (keys_held >= HID_BOOT_KEY_LEN) { \
        error_report.modifiers = keyboard_report.body.modifiers; \
        return &error_report; \
    }

#endif

#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_NKRO)

#define TOGGLE_KEYBOARD(code, val) WRITE_BIT(keyboard_report.body.keys[code / 8], code % 8, val)

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
static zmk_hid_boot_report_t *get_boot_report() {
    HID_CHECK_ROLLOVER_ERROR();

    boot_report.modifiers = keyboard_report.body.modifiers;
    memset(&boot_report.keys, 0, HID_BOOT_KEY_LEN);
    int ix = 0;
    uint8_t base_code = 0;
    for (int i = 0; i < (ZMK_HID_KEYBOARD_NKRO_MAX_USAGE + 1) / 8; ++i) {
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
static zmk_hid_boot_report_t *get_boot_report() {
    HID_CHECK_ROLLOVER_ERROR();
#if CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE != HID_BOOT_KEY_LEN
    boot_report.modifiers = keyboard_report.body.modifiers;
    int len = CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE < HID_BOOT_KEY_LEN ? CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE : HID_BOOT_KEY_LEN;
    memcpy(&boot_report.keys, keyboard_report.body.keys, len);
    return &boot_report;
#else
    return &keyboard_report.body;
#endif
}
#endif

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
    zmk_mod_flags_t current = GET_MODIFIERS;
    SET_MODIFIERS(explicit_modifiers | implicit_modifiers);
    return current == GET_MODIFIERS ? 0 : 1;
}

int zmk_hid_implicit_modifiers_release() {
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

uint8_t *zmk_hid_get_keyboard_report(zmk_hid_report_request_t req_t, uint8_t proto) {
    if (proto == HID_PROTOCOL_REPORT) {
        return req_t == HID_REPORT_FULL ? (uint8_t*)&keyboard_report : (uint8_t*)&keyboard_report.body;
    }
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    return (uint8_t*)get_boot_report();
#else
    return NULL;
#endif
}

uint8_t *zmk_hid_get_consumer_report(zmk_hid_report_request_t req_t) {
    return req_t == HID_REPORT_FULL ? (uint8_t*)&consumer_report : (uint8_t*)&consumer_report.body;
}
