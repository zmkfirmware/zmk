/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_soft_off

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/pm.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_soft_off_config {
    uint32_t hold_time_ms;
};

struct behavior_soft_off_data {
    uint32_t press_start;
};

static int behavior_soft_off_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_soft_off_data *data = dev->data;

#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zmk_pm_soft_off();
#else
    data->press_start = k_uptime_get();
#endif

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_soft_off_data *data = dev->data;
    const struct behavior_soft_off_config *config = dev->config;

    if (config->hold_time_ms == 0 || (k_uptime_get() - data->press_start) >= config->hold_time_ms) {
        zmk_pm_soft_off();
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_soft_off_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

#define BSO_INST(n)                                                                                \
    static const struct behavior_soft_off_config bso_config_##n = {                                \
        .hold_time_ms = DT_INST_PROP_OR(n, hold_time_ms, 0),                                       \
    };                                                                                             \
    static struct behavior_soft_off_data bso_data_##n = {};                                        \
    BEHAVIOR_DT_INST_DEFINE(0, behavior_soft_off_init, NULL, &bso_data_##n, &bso_config_##n,       \
                            APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_soft_off_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BSO_INST)
