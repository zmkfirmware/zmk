/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_toggle_layer

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_tog_config {};
struct behavior_tog_data {};

static int behavior_tog_init(const struct device *dev) { return 0; };

static int tog_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return zmk_keymap_layer_toggle(binding->param1);
}

static int tog_keymap_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_tog_driver_api = {
    .binding_pressed = tog_keymap_binding_pressed,
    .binding_released = tog_keymap_binding_released,
};

static const struct behavior_tog_config behavior_tog_config = {};

static struct behavior_tog_data behavior_tog_data;

DEVICE_AND_API_INIT(behavior_tog, DT_INST_LABEL(0), behavior_tog_init, &behavior_tog_data,
                    &behavior_tog_config, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_tog_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
