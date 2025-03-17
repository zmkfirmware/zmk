/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_underglow_indicators

// Dependencies
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/hid_indicators.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/underglow_color_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct underglow_indicators_data {
    zmk_hid_indicators_t indicators;
};

struct underglow_indicators_config {
    int indicator;
};

static int underglow_indicators_init(const struct device *dev) { return 0; };

static int underglow_indicators_process(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct underglow_indicators_data *data = dev->data;
    const struct underglow_indicators_config *config = dev->config;

    if (data->indicators & BIT(config->indicator))
        return binding->param2;
    else
        return binding->param1;
}

static const struct behavior_driver_api underglow_indicators_driver_api = {
    .binding_pressed = underglow_indicators_process,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

static int underglow_indicators_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_underglow_indicators, underglow_indicators_listener);
ZMK_SUBSCRIPTION(behavior_underglow_indicators, zmk_hid_indicators_changed);

static struct underglow_indicators_data underglow_indicators_data = {.indicators = 0};

static int underglow_indicators_listener(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);
    underglow_indicators_data.indicators = ev->indicators;
    raise_zmk_underglow_color_changed((struct zmk_underglow_color_changed){});

    return ZMK_EV_EVENT_BUBBLE;
}

#define KP_INST(n)                                                                                 \
    static struct underglow_indicators_config underglow_indicators_config_##n = {                  \
        .indicator = DT_INST_PROP(n, indicator)};                                                  \
    BEHAVIOR_DT_INST_DEFINE(n, underglow_indicators_init, NULL, &underglow_indicators_data,        \
                            &underglow_indicators_config_##n, POST_KERNEL,                         \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &underglow_indicators_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
