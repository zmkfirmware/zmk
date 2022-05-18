/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_ext_power

#include <device.h>
#include <devicetree.h>
#include <drivers/behavior.h>
#include <zmk/power_domain.h>

#include <dt-bindings/zmk/ext_power.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct zmk_behavior_ext_power_config {
    const struct device *pd;
};

static int
on_keymap_binding_convert_central_state_dependent_params(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct zmk_behavior_ext_power_config *config = (struct zmk_behavior_ext_power_config *)dev->config;

    if (binding->param1 == EXT_POWER_TOGGLE_CMD) {
        binding->param1 = zmk_power_domain_get_state_actual(config->pd) == 1 ? EXT_POWER_OFF_CMD : EXT_POWER_ON_CMD;
    }

    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {

    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct zmk_behavior_ext_power_config *config = (struct zmk_behavior_ext_power_config *)dev->config;

    switch (binding->param1) {
    case EXT_POWER_OFF_CMD:
        return zmk_power_domain_disable(config->pd, true);
    case EXT_POWER_ON_CMD:
        return zmk_power_domain_enable(config->pd, true);
    case EXT_POWER_TOGGLE_CMD:
        zmk_power_domain_toggle(config->pd, true);
    default:
        LOG_ERR("Unknown ext_power command: %d", binding->param1);
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_ext_power_init(const struct device *dev) {
    return 0;
};

static const struct behavior_driver_api behavior_ext_power_driver_api = {
    .binding_convert_central_state_dependent_params =
        on_keymap_binding_convert_central_state_dependent_params,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

#define KP_INST(n) \
    static struct zmk_behavior_ext_power_config zmk_behavior_ext_power_config_##n = { \
        .pd = COND_CODE_1( \
            DT_INST_NODE_HAS_PROP(n, power_domain), \
            DEVICE_DT_GET(DT_INST_PHANDLE(n, power_domain)), \
            NULL \
        ), \
    }; \
    DEVICE_DT_INST_DEFINE( \
        n, \
        behavior_ext_power_init, \
        NULL, \
        NULL, \
        &zmk_behavior_ext_power_config_##n, \
        APPLICATION, \
        CONFIG_APPLICATION_INIT_PRIORITY, \
        &behavior_ext_power_driver_api \
    );

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
