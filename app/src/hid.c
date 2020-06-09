#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/hid.h>

static struct zmk_hid_keypad_report kp_report = {
    .report_id = 1,
    .body = {
        .modifiers = 0,
        .keys = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};

static struct zmk_hid_consumer_report consumer_report = {
    .report_id = 2,
    .body = {
        .keys = 0x00}};

#define _TOGGLE_MOD(mod, state)                      \
    if (modifier > MOD_RGUI)                         \
    {                                                \
        return -EINVAL;                              \
    }                                                \
    WRITE_BIT(kp_report.body.modifiers, mod, state); \
    return 0;

int zmk_hid_register_mod(zmk_mod modifier)
{
    _TOGGLE_MOD(modifier, true);
}
int zmk_hid_unregister_mod(zmk_mod modifier)
{
    _TOGGLE_MOD(modifier, false);
}

int zmk_hid_register_mods(zmk_mod_flags modifiers)
{
    kp_report.body.modifiers |= modifiers;
    return 0;
}

int zmk_hid_unregister_mods(zmk_mod_flags modifiers)
{
    kp_report.body.modifiers &= ~modifiers;
    return 0;
}

#define KEY_OFFSET 0x02
#define MAX_KEYS 6

/*
#define TOGGLE_BOOT_KEY(match, val)                      \
    for (int idx = 0; idx < MAX_KEYS; idx++)             \
    {                                                    \
        if (kp_report.boot.keys[idx + KEY_OFFSET] != match) \
        {                                                \
            continue;                                    \
        }                                                \
        kp_report.boot.keys[idx + KEY_OFFSET] = val;        \
        break;                                           \
    }
*/

#define TOGGLE_KEY(code, val) WRITE_BIT(kp_report.body.keys[code / 8], code % 8, val)

#define TOGGLE_CONSUMER(key, state) \
    WRITE_BIT(consumer_report.body.keys, (key - 0x100), state);

enum zmk_hid_report_changes zmk_hid_press_key(zmk_key code)
{
    if (code >= KC_LCTL && code <= KC_RGUI)
    {
        return zmk_hid_register_mod(code - KC_LCTL);
    }

    if (ZK_IS_CONSUMER(code))
    {
        LOG_DBG("Toggling a consumer key!");
        TOGGLE_CONSUMER(code, true);
        return Consumer;
    }
    else
    {
        if (code > ZMK_HID_MAX_KEYCODE)
        {
            return -EINVAL;
        }

        // TOGGLE_BOOT_KEY(0U, code);

        TOGGLE_KEY(code, true);

        return Keypad;
    }
};

enum zmk_hid_report_changes zmk_hid_release_key(zmk_key code)
{
    if (code >= KC_LCTL && code <= KC_RGUI)
    {
        return zmk_hid_unregister_mod(code - KC_LCTL);
    }

    if (ZK_IS_CONSUMER(code))
    {
        TOGGLE_CONSUMER(code, false);
        return Consumer;
    }
    else
    {
        if (code > ZMK_HID_MAX_KEYCODE)
        {
            return -EINVAL;
        }

        // TOGGLE_BOOT_KEY(0U, code);

        TOGGLE_KEY(code, false);

        return Keypad;
    }
};

struct zmk_hid_keypad_report *zmk_hid_get_keypad_report()
{
    return &kp_report;
}

struct zmk_hid_consumer_report *zmk_hid_get_consumer_report()
{
    return &consumer_report;
}
