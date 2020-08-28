/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_bluetooth

#include <device.h>
#include <drivers/behavior.h>

#include <dt-bindings/zmk/bt.h>

#include <bluetooth/conn.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>

static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t command, u32_t arg)
{
    switch (command)
    {
    case BT_RST_CMD:
        return zmk_ble_unpair_all();
    case BT_IDENT_CLR_CMD:
        return zmk_ble_identity_clear();
#if CONFIG_BT_ID_MAX != 1
    case BT_IDENT_NEXT_CMD:
        return zmk_ble_identity_next();
    case BT_IDENT_PREV_CMD:
        return zmk_ble_identity_prev();
    case BT_IDENT_SEL_CMD:
        return zmk_ble_identity_select(arg);
#endif /* BT_ID_MAX != 1 */
    }

    return -ENOTSUP;
}

static int behavior_bt_init(struct device *dev)
{
    return 0;
};

static int on_keymap_binding_released(struct device *dev, u32_t position, u32_t command, u32_t arg)
{
    return 0;
}

static const struct behavior_driver_api behavior_bt_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

DEVICE_AND_API_INIT(behavior_bt, DT_INST_LABEL(0),
                    behavior_bt_init,
                    NULL,
                    NULL,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_bt_driver_api);
