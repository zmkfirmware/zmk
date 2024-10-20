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
    bool split_peripheral_turn_off_on_press;
    uint32_t hold_time_ms;
};

struct behavior_soft_off_data {
    uint32_t press_start;
};

#define IS_SPLIT_PERIPHERAL                                                                        \
    (IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

static int behavior_soft_off_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_soft_off_data *data = dev->data;
    const struct behavior_soft_off_config *config = dev->config;

    if (IS_SPLIT_PERIPHERAL && config->split_peripheral_turn_off_on_press) {
        zmk_pm_soft_off();
    } else {
        data->press_start = k_uptime_get();
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_soft_off_data *data = dev->data;
    const struct behavior_soft_off_config *config = dev->config;

    if (config->hold_time_ms == 0) {
        LOG_DBG("No hold time set, triggering soft off");
        zmk_pm_soft_off();
    } else {
        uint32_t hold_time = k_uptime_get() - data->press_start;

        if (hold_time > config->hold_time_ms) {
            if (IS_ENABLED(CONFIG_ZMK_SPLIT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)) {
                k_sleep(K_MSEC(100));
            }
            zmk_pm_soft_off();
        } else {
            LOG_INF("Not triggering soft off: held for %d and hold time is %d", hold_time,
                    config->hold_time_ms);
        }
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_soft_off_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define BSO_INST(n)                                                                                \
    static const struct behavior_soft_off_config bso_config_##n = {                                \
        .hold_time_ms = DT_INST_PROP_OR(n, hold_time_ms, 0),                                       \
        .split_peripheral_turn_off_on_press =                                                      \
            DT_INST_PROP_OR(n, split_peripheral_off_on_press, false),                              \
    };                                                                                             \
    static struct behavior_soft_off_data bso_data_##n = {};                                        \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_soft_off_init, NULL, &bso_data_##n, &bso_config_##n,       \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_soft_off_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BSO_INST)
