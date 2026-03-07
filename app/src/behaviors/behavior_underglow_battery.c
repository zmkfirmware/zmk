/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_underglow_battery

// Dependencies
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/battery.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/underglow_color_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct underglow_battery_data {
    uint32_t layers;
};

struct underglow_battery_config {
    int threshold;
};

static struct underglow_battery_data underglow_battery_data = {.layers = 0};

static int underglow_battery_init(const struct device *dev) { return 0; };

static int underglow_battery_process(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct underglow_battery_config *config = dev->config;
    struct underglow_battery_data *data = dev->data;
    data->layers |= BIT(event.layer);
    int bat = zmk_battery_state_of_charge();

    if (bat >= config->threshold)
        return binding->param2;
    else
        return binding->param1;
}

static const struct behavior_driver_api underglow_battery_driver_api = {
    .binding_pressed = underglow_battery_process,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

static int underglow_battery_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_underglow_battery, underglow_battery_listener);
ZMK_SUBSCRIPTION(behavior_underglow_battery, zmk_battery_state_changed);

static int underglow_battery_listener(const zmk_event_t *eh) {
    raise_zmk_underglow_color_changed((struct zmk_underglow_color_changed){
        .layers = underglow_battery_data.layers, .wakeup = false});

    return ZMK_EV_EVENT_BUBBLE;
}

#define KP_INST(n)                                                                                 \
    static struct underglow_battery_config underglow_battery_config_##n = {                        \
        .threshold = DT_INST_PROP(n, threshold)};                                                  \
    BEHAVIOR_DT_INST_DEFINE(n, underglow_battery_init, NULL, &underglow_battery_data,              \
                            &underglow_battery_config_##n, POST_KERNEL,                            \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &underglow_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
