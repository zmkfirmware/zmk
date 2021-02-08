/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_bluetooth

#include <device.h>
#include <drivers/behavior.h>
#include <dt-bindings/zmk/bt.h>
#include <bluetooth/conn.h>
#include <logging/log.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case BT_CLR_CMD:
        return zmk_ble_clear_bonds();
    case BT_NXT_CMD:
        return zmk_ble_prof_next();
    case BT_PRV_CMD:
        return zmk_ble_prof_prev();
    case BT_SEL_CMD:
        return zmk_ble_prof_select(binding->param2);
    default:
        LOG_ERR("Unknown BT command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static int behavior_bt_init(const struct device *dev) { return 0; };

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_bt_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

DEVICE_AND_API_INIT(behavior_bt, DT_INST_LABEL(0), behavior_bt_init, NULL, NULL, APPLICATION,
                    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_bt_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */