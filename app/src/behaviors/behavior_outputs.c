/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_outputs

#include <device.h>
#include <devicetree.h>
#include <drivers/behavior.h>

#include <dt-bindings/zmk/outputs.h>

#include <zmk/behavior.h>
#include <zmk/endpoints.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case OUT_TOG:
        return zmk_endpoints_toggle();
    case OUT_USB:
        return zmk_endpoints_select(ZMK_ENDPOINT_USB);
    case OUT_BLE:
        return zmk_endpoints_select(ZMK_ENDPOINT_BLE);
    default:
        LOG_ERR("Unknown output command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static int behavior_out_init(const struct device *dev) { return 0; }

static const struct behavior_driver_api behavior_outputs_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
};

DEVICE_AND_API_INIT(behavior_out, DT_INST_LABEL(0), behavior_out_init, NULL, NULL, APPLICATION,
                    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_outputs_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
