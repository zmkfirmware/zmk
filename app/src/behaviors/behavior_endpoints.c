/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_endpoints

#include <device.h>
#include <devicetree.h>
#include <drivers/behavior.h>

#include <dt-bindings/zmk/endpoints.h>

#include <zmk/behavior.h>
#include <zmk/endpoints.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case ENDPOINT_TOGGLE_CMD:
        return zmk_endpoints_toggle();
    case ENDPOINT_USB_CMD:
        return zmk_endpoints_select(ZMK_ENDPOINT_USB);
    case ENDPOINT_BLE_CMD:
        return zmk_endpoints_select(ZMK_ENDPOINT_BLE);
    default:
        LOG_ERR("Unknown endpoints command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static int behavior_ep_init(struct device *dev) { return 0; }

static const struct behavior_driver_api behavior_endpoints_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
};

DEVICE_AND_API_INIT(behavior_end, DT_INST_LABEL(0), behavior_ep_init, NULL, NULL, APPLICATION,
                    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_endpoints_driver_api);
