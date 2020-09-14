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
    case BT_CLR_CMD:
        return zmk_ble_clear_bonds();
    case BT_NXT_CMD:
        return zmk_ble_prof_next();
    case BT_PRV_CMD:
        return zmk_ble_prof_prev();
    case BT_SEL_CMD:
        return zmk_ble_prof_select(arg);
    default:
        LOG_ERR("Unknown BT command: %d", command);
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
