/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "zmk/keys.h"
#include <stddef.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/hid.h>
#include <dt-bindings/zmk/modifiers.h>

static struct zmk_hid_keyboard_report keyboard_report = {
    .report_id = ZMK_HID_REPORT_ID_KEYBOARD,
#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_DYNAMIC)
    // .keys doesn't exist for the dynamic union body; naming either member zero-initializes
    // the whole union the same way the static storage duration default already would.
    .body = {.modifiers = 0, ._reserved = 0, .nkro_keys = {0}},
#else
    .body = {.modifiers = 0, ._reserved = 0, .keys = {0}},
#endif
};

static struct zmk_hid_consumer_report consumer_report = {.report_id = ZMK_HID_REPORT_ID_CONSUMER,
                                                         .body = {.keys = {0}}};

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)

static zmk_hid_boot_report_t boot_report = {.modifiers = 0, ._reserved = 0, .keys = {0}};
static uint8_t keys_held = 0;

#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

#if IS_ENABLED(CONFIG_ZMK_POINTING)

static struct zmk_hid_mouse_report mouse_report = {
    .report_id = ZMK_HID_REPORT_ID_MOUSE,
    .body = {.buttons = 0, .d_x = 0, .d_y = 0, .d_scroll_y = 0, .d_scroll_x = 0}};

#endif // IS_ENABLED(CONFIG_ZMK_POINTING)

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

// --- Shared keyboard-usage primitives ---------------------------------------------------------
//
// NKRO (bitmap body) and HKRO (array body) each need exactly one implementation of
// press / release / is-pressed / boot-report conversion. Every report-type configuration is just
// a binding of these primitives to a key buffer: the static NKRO/HKRO configs bind the single
// body.keys buffer at compile time; the dynamic config binds body.nkro_keys or body.hkro_keys per
// the boot-resolved mode. Keeping the loops here means each is written (and audited) once,
// regardless of how many modes are compiled in. keys_held bookkeeping stays in the public
// dispatchers below so its increment/decrement ordering is unchanged.

#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_NKRO) || IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_DYNAMIC)

static inline int nkro_toggle(uint8_t *keys, zmk_key_t usage, int val) {
    if (usage > ZMK_HID_KEYBOARD_NKRO_MAX_USAGE) {
        return -EINVAL;
    }
    WRITE_BIT(keys[usage / 8], usage % 8, val);
    return 0;
}

static inline bool nkro_check(const uint8_t *keys, zmk_key_t usage) {
    if (usage > ZMK_HID_KEYBOARD_NKRO_MAX_USAGE) {
        return false;
    }
    return keys[usage / 8] & BIT(usage % 8);
}

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
static inline zmk_hid_boot_report_t *nkro_boot_report(const uint8_t *keys, size_t len) {
    memset(&boot_report.keys, 0, HID_BOOT_KEY_LEN);
    int ix = 0;
    for (int i = 0; i < len; ++i) {
        if (ix == keys_held) {
            break;
        }
        if (!keys[i]) {
            continue;
        }
        uint8_t base_code = i * 8;
        for (int j = 0; j < 8; ++j) {
            if (keys[i] & BIT(j)) {
                boot_report.keys[ix++] = base_code + j;
            }
        }
    }
    return &boot_report;
}
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

#endif /* NKRO || DYNAMIC */

#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_HKRO) || IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_DYNAMIC)

static inline void hkro_toggle(uint8_t *keys, size_t size, uint8_t match, uint8_t val) {
    for (int idx = 0; idx < size; idx++) {
        if (keys[idx] != match) {
            continue;
        }
        keys[idx] = val;
        if (val) {
            break;
        }
    }
}

static inline bool hkro_check(const uint8_t *keys, size_t size, zmk_key_t usage) {
    for (int idx = 0; idx < size; idx++) {
        if (keys[idx] == usage) {
            return true;
        }
    }
    return false;
}

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
static inline zmk_hid_boot_report_t *hkro_boot_report(const uint8_t *keys, size_t size) {
    int out = 0;
    for (int i = 0; i < size; i++) {
        uint8_t key = keys[i];
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
}
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

#endif /* HKRO || DYNAMIC */

#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_NKRO) && !IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_DYNAMIC)

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
zmk_hid_boot_report_t *zmk_hid_get_boot_report(void) {
    if (keys_held > HID_BOOT_KEY_LEN) {
        return boot_report_rollover(keyboard_report.body.modifiers);
    }

    boot_report.modifiers = keyboard_report.body.modifiers;
    return nkro_boot_report(keyboard_report.body.keys, sizeof(keyboard_report.body.keys));
}
#endif

static inline int select_keyboard_usage(zmk_key_t usage) {
    int ret = nkro_toggle(keyboard_report.body.keys, usage, 1);
    if (ret) {
        return ret;
    }
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    ++keys_held;
#endif
    return 0;
}

static inline int deselect_keyboard_usage(zmk_key_t usage) {
    int ret = nkro_toggle(keyboard_report.body.keys, usage, 0);
    if (ret) {
        return ret;
    }
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    --keys_held;
#endif
    return 0;
}

static inline bool check_keyboard_usage(zmk_key_t usage) {
    return nkro_check(keyboard_report.body.keys, usage);
}

const uint8_t *zmk_hid_report_desc_get(size_t *len) {
    *len = sizeof(zmk_hid_report_desc);
    return zmk_hid_report_desc;
}

size_t zmk_hid_keyboard_report_body_size(void) {
    return sizeof(struct zmk_hid_keyboard_report_body);
}

#elif IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_HKRO) && !IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_DYNAMIC)

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
zmk_hid_boot_report_t *zmk_hid_get_boot_report(void) {
    if (keys_held > HID_BOOT_KEY_LEN) {
        return boot_report_rollover(keyboard_report.body.modifiers);
    }

#if CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE != HID_BOOT_KEY_LEN
    // Form a boot report from a report of different size.
    boot_report.modifiers = keyboard_report.body.modifiers;
    return hkro_boot_report(keyboard_report.body.keys, CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE);
#else
    return &keyboard_report.body;
#endif /* CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE != HID_BOOT_KEY_LEN */
}
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

static inline int select_keyboard_usage(zmk_key_t usage) {
    hkro_toggle(keyboard_report.body.keys, CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE, 0U, usage);
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    ++keys_held;
#endif
    return 0;
}

static inline int deselect_keyboard_usage(zmk_key_t usage) {
    hkro_toggle(keyboard_report.body.keys, CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE, usage, 0U);
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    --keys_held;
#endif
    return 0;
}

static inline int check_keyboard_usage(zmk_key_t usage) {
    return hkro_check(keyboard_report.body.keys, CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE, usage);
}

const uint8_t *zmk_hid_report_desc_get(size_t *len) {
    *len = sizeof(zmk_hid_report_desc);
    return zmk_hid_report_desc;
}

size_t zmk_hid_keyboard_report_body_size(void) {
    return sizeof(struct zmk_hid_keyboard_report_body);
}

#elif IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_DYNAMIC)

static inline bool dynamic_nkro_active(void) {
    return zmk_hid_dynamic_nkro_get_mode() == ZMK_HID_DYNAMIC_NKRO_MODE_NKRO;
}

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
zmk_hid_boot_report_t *zmk_hid_get_boot_report(void) {
    if (keys_held > HID_BOOT_KEY_LEN) {
        return boot_report_rollover(keyboard_report.body.modifiers);
    }

    boot_report.modifiers = keyboard_report.body.modifiers;

    if (dynamic_nkro_active()) {
        return nkro_boot_report(keyboard_report.body.nkro_keys,
                                sizeof(keyboard_report.body.nkro_keys));
    }
    return hkro_boot_report(keyboard_report.body.hkro_keys, CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE);
}
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

static inline int select_keyboard_usage(zmk_key_t usage) {
    if (dynamic_nkro_active()) {
        int ret = nkro_toggle(keyboard_report.body.nkro_keys, usage, 1);
        if (ret) {
            return ret;
        }
    } else {
        hkro_toggle(keyboard_report.body.hkro_keys, CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE, 0U, usage);
    }
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    ++keys_held;
#endif
    return 0;
}

static inline int deselect_keyboard_usage(zmk_key_t usage) {
    if (dynamic_nkro_active()) {
        int ret = nkro_toggle(keyboard_report.body.nkro_keys, usage, 0);
        if (ret) {
            return ret;
        }
    } else {
        hkro_toggle(keyboard_report.body.hkro_keys, CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE, usage, 0U);
    }
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    --keys_held;
#endif
    return 0;
}

static inline bool check_keyboard_usage(zmk_key_t usage) {
    if (dynamic_nkro_active()) {
        return nkro_check(keyboard_report.body.nkro_keys, usage);
    }
    return hkro_check(keyboard_report.body.hkro_keys, CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE, usage);
}

const uint8_t *zmk_hid_report_desc_get(size_t *len) {
    if (dynamic_nkro_active()) {
        *len = sizeof(zmk_hid_report_desc_nkro);
        return zmk_hid_report_desc_nkro;
    }

    *len = sizeof(zmk_hid_report_desc_hkro);
    return zmk_hid_report_desc_hkro;
}

size_t zmk_hid_keyboard_report_body_size(void) {
    size_t keys_len = dynamic_nkro_active()
                          ? sizeof(((struct zmk_hid_keyboard_report_body *)NULL)->nkro_keys)
                          : sizeof(((struct zmk_hid_keyboard_report_body *)NULL)->hkro_keys);
    return offsetof(struct zmk_hid_keyboard_report_body, nkro_keys) + keys_len;
}

#else
#error "A proper HID report type must be selected"
#endif

#define TOGGLE_CONSUMER(match, val)                                                                \
    if (val > ZMK_HID_CONSUMER_MAX_USAGE) {                                                        \
        return -ENOTSUP;                                                                           \
    }                                                                                              \
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

#if IS_ENABLED(CONFIG_ZMK_POINTING)

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

void zmk_hid_mouse_movement_set(int16_t hwheel, int16_t wheel) {
    mouse_report.body.d_x = hwheel;
    mouse_report.body.d_y = wheel;
    LOG_DBG("Mouse movement set to %d/%d", mouse_report.body.d_x, mouse_report.body.d_y);
}

void zmk_hid_mouse_movement_update(int16_t hwheel, int16_t wheel) {
    mouse_report.body.d_x += hwheel;
    mouse_report.body.d_y += wheel;
    LOG_DBG("Mouse movement updated to %d/%d", mouse_report.body.d_x, mouse_report.body.d_y);
}

void zmk_hid_mouse_scroll_set(int16_t hwheel, int16_t wheel) {
    mouse_report.body.d_scroll_x = hwheel;
    mouse_report.body.d_scroll_y = wheel;

    LOG_DBG("Mouse scroll set to %d/%d", mouse_report.body.d_scroll_x,
            mouse_report.body.d_scroll_y);
}

void zmk_hid_mouse_scroll_update(int16_t hwheel, int16_t wheel) {
    mouse_report.body.d_scroll_x += hwheel;
    mouse_report.body.d_scroll_y += wheel;

    LOG_DBG("Mouse scroll updated to X: %d/%d", mouse_report.body.d_scroll_x,
            mouse_report.body.d_scroll_y);
}

void zmk_hid_mouse_clear(void) {
    LOG_DBG("Mouse report cleared");
    memset(&mouse_report.body, 0, sizeof(mouse_report.body));
}

#endif // IS_ENABLED(CONFIG_ZMK_POINTING)

struct zmk_hid_keyboard_report *zmk_hid_get_keyboard_report(void) { return &keyboard_report; }

struct zmk_hid_consumer_report *zmk_hid_get_consumer_report(void) { return &consumer_report; }

#if IS_ENABLED(CONFIG_ZMK_POINTING)

struct zmk_hid_mouse_report *zmk_hid_get_mouse_report(void) { return &mouse_report; }

#endif // IS_ENABLED(CONFIG_ZMK_POINTING)
