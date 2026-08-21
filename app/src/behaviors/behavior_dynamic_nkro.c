/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_dynamic_nkro

#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <dt-bindings/zmk/dynamic_nkro.h>

#include <zmk/hid_dynamic_nkro.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    enum zmk_hid_dynamic_nkro_mode current = zmk_hid_dynamic_nkro_get_mode();
    enum zmk_hid_dynamic_nkro_mode target;

    switch (binding->param1) {
    case DYN_NKRO_TOG:
        target = current == ZMK_HID_DYNAMIC_NKRO_MODE_NKRO ? ZMK_HID_DYNAMIC_NKRO_MODE_HKRO
                                                           : ZMK_HID_DYNAMIC_NKRO_MODE_NKRO;
        break;
    case DYN_NKRO_NKRO:
        target = ZMK_HID_DYNAMIC_NKRO_MODE_NKRO;
        break;
    case DYN_NKRO_HKRO:
        target = ZMK_HID_DYNAMIC_NKRO_MODE_HKRO;
        break;
    default:
        LOG_ERR("Unknown dynamic NKRO command: %d", binding->param1);
        return -ENOTSUP;
    }

    if (target == current) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    int ret = zmk_hid_dynamic_nkro_set_mode(target);
    if (ret < 0) {
        return ret;
    }

    LOG_INF("Dynamic NKRO mode persisted, rebooting to apply it");

    // The descriptor registered with USB/BLE HID can't change without a full
    // re-enumeration, so the new mode only takes effect after this reboot.
    sys_reboot(SYS_REBOOT_WARM);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_dynamic_nkro_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_dynamic_nkro_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
