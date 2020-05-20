#include <zmk/hid.h>

static struct zmk_hid_report report = {
    .modifiers = 0,
    .keys = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

#define _TOGGLE_MOD(mod, state)              \
    if (modifier > MOD_RGUI)                 \
    {                                        \
        return -EINVAL;                      \
    }                                        \
    WRITE_BIT(report.modifiers, mod, state); \
    return 0;

int zmk_hid_register_mod(zmk_mod modifier)
{
    _TOGGLE_MOD(modifier, true);
}
int zmk_hid_unregister_mod(zmk_mod modifier)
{
    _TOGGLE_MOD(modifier, false);
}

#define KEY_OFFSET 0x02
#define MAX_KEYS 6

/*
#define TOGGLE_BOOT_KEY(match, val)                      \
    for (int idx = 0; idx < MAX_KEYS; idx++)             \
    {                                                    \
        if (report.boot.keys[idx + KEY_OFFSET] != match) \
        {                                                \
            continue;                                    \
        }                                                \
        report.boot.keys[idx + KEY_OFFSET] = val;        \
        break;                                           \
    }
*/

#define TOGGLE_KEY(code, val) WRITE_BIT(report.keys[code / 8], code % 8, val)

int zmk_hid_press_key(zmk_key code)
{
    if (code >= KC_LCTL && code <= KC_RGUI)
    {
        return zmk_hid_register_mod(code - KC_LCTL);
    }

    if (code > ZMK_HID_MAX_KEYCODE)
    {
        return -EINVAL;
    }

    // TOGGLE_BOOT_KEY(0U, code);

    TOGGLE_KEY(code, true);

    return 0;
};

int zmk_hid_release_key(zmk_key code)
{
    if (code >= KC_LCTL && code <= KC_RGUI)
    {
        return zmk_hid_unregister_mod(code - KC_LCTL);
    }

    if (code > ZMK_HID_MAX_KEYCODE)
    {
        return -EINVAL;
    }

    // TOGGLE_BOOT_KEY(code, 0U);

    TOGGLE_KEY(code, false);

    return 0;
};

struct zmk_hid_report *zmk_hid_get_report()
{
    return &report;
}
