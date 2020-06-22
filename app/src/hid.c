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
        .keys = {0,0,0,0,0,0}}};

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

#define TOGGLE_CONSUMER(match, val)                  \
    for (int idx = 0; idx < MAX_KEYS; idx++)         \
    {                                                \
        if (consumer_report.body.keys[idx] != match) \
        {                                            \
            continue;                                \
        }                                            \
        consumer_report.body.keys[idx] = val;        \
        break;                                       \
    }


int zmk_hid_keypad_press(zmk_key code)
{
    if (code >= LCTL && code <= RGUI)
    {
        return zmk_hid_register_mod(code - LCTL);
    }

   
    if (code > ZMK_HID_MAX_KEYCODE)
    {
        return -EINVAL;
    }

    // TOGGLE_BOOT_KEY(0U, code);

    TOGGLE_KEY(code, true);

    return 0;
};

int zmk_hid_keypad_release(zmk_key code)
{
    if (code >= LCTL && code <= RGUI)
    {
        return zmk_hid_unregister_mod(code - LCTL);
    }

    if (code > ZMK_HID_MAX_KEYCODE)
    {
        return -EINVAL;
    }

    // TOGGLE_BOOT_KEY(0U, code);

    TOGGLE_KEY(code, false);

    return 0;
};

int zmk_hid_consumer_press(zmk_key code)
{
    TOGGLE_CONSUMER(0U, code);
    return 0;
};


int zmk_hid_consumer_release(zmk_key code)
{
    TOGGLE_CONSUMER(code, 0U);
    return 0;
};

struct zmk_hid_keypad_report *zmk_hid_get_keypad_report()
{
    return &kp_report;
}

struct zmk_hid_consumer_report *zmk_hid_get_consumer_report()
{
    return &consumer_report;
}
