/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/hid_dynamic_nkro.h>

// First-boot default (before any mode is persisted) follows the HID Report Type choice:
// CONFIG_ZMK_HID_REPORT_TYPE_NKRO -> NKRO, otherwise HKRO (ZMK's own choice default).
#if IS_ENABLED(CONFIG_ZMK_HID_REPORT_TYPE_NKRO)
#define ZMK_HID_DYNAMIC_NKRO_DEFAULT_MODE ZMK_HID_DYNAMIC_NKRO_MODE_NKRO
#else
#define ZMK_HID_DYNAMIC_NKRO_DEFAULT_MODE ZMK_HID_DYNAMIC_NKRO_MODE_HKRO
#endif

static enum zmk_hid_dynamic_nkro_mode current_mode = ZMK_HID_DYNAMIC_NKRO_DEFAULT_MODE;

enum zmk_hid_dynamic_nkro_mode zmk_hid_dynamic_nkro_get_mode(void) { return current_mode; }

int zmk_hid_dynamic_nkro_set_mode(enum zmk_hid_dynamic_nkro_mode mode) {
    if (mode == current_mode) {
        return 0;
    }

    current_mode = mode;

    uint8_t stored = (uint8_t)mode;
    int ret = settings_save_one("zmk/hid/mode", &stored, sizeof(stored));
    if (ret < 0) {
        LOG_ERR("Failed to persist dynamic NKRO mode (%d)", ret);
    }

    return ret;
}

static int hid_dynamic_nkro_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                         void *cb_arg) {
    const char *next;

    if (settings_name_steq(name, "mode", &next) && !next) {
        uint8_t stored;

        if (len != sizeof(stored)) {
            return -EINVAL;
        }

        int ret = read_cb(cb_arg, &stored, sizeof(stored));
        if (ret <= 0) {
            return ret;
        }

        if (stored != ZMK_HID_DYNAMIC_NKRO_MODE_NKRO && stored != ZMK_HID_DYNAMIC_NKRO_MODE_HKRO) {
            LOG_WRN("Ignoring invalid stored dynamic NKRO mode %d", stored);
            return -EINVAL;
        }

        current_mode = (enum zmk_hid_dynamic_nkro_mode)stored;
        LOG_INF("Loaded dynamic NKRO mode %d from settings", current_mode);
    }

    return 0;
}

static struct settings_handler hid_dynamic_nkro_settings_handler = {
    .name = "zmk/hid",
    .h_set = hid_dynamic_nkro_settings_set,
};

static int hid_dynamic_nkro_init(void) {
    settings_subsys_init();

    int ret = settings_register(&hid_dynamic_nkro_settings_handler);
    if (ret < 0) {
        LOG_ERR("Failed to register dynamic NKRO settings handler (%d)", ret);
        return ret;
    }

    /*
     * USB HID (prio CONFIG_ZMK_USB_HID_INIT_PRIORITY) and BLE HOG (prio
     * CONFIG_ZMK_BLE_INIT_PRIORITY) both register a fixed report descriptor/size for the rest
     * of the boot session, so the persisted mode has to be loaded eagerly here rather than
     * waiting for the application-level settings_load() in main().
     */
    ret = settings_load_subtree("zmk/hid");
    if (ret < 0) {
        LOG_ERR("Failed to load dynamic NKRO settings (%d)", ret);
        return ret;
    }

    LOG_INF("Dynamic NKRO active mode for this boot: %s",
            current_mode == ZMK_HID_DYNAMIC_NKRO_MODE_NKRO ? "NKRO" : "HKRO");

    return 0;
}

SYS_INIT(hid_dynamic_nkro_init, APPLICATION, CONFIG_ZMK_HID_REPORT_TYPE_DYNAMIC_INIT_PRIORITY);
