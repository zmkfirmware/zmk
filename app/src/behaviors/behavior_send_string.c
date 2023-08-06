/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_send_string

#include <drivers/behavior.h>
#include <drivers/character_map.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zmk/behavior.h>
#include <zmk/send_string.h>

struct behavior_send_string_config {
    const char *text;
    struct zmk_send_string_config config;
};

static int on_send_string_binding_pressed(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_send_string_config *config = dev->config;

    zmk_send_string(&config->config, &event, config->text);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_send_string_binding_released(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_send_string_driver_api = {
    .binding_pressed = on_send_string_binding_pressed,
    .binding_released = on_send_string_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

static int behavior_send_string_init(const struct device *dev) { return 0; }

#define SEND_STRING_INST(n)                                                                        \
    ZMK_BUILD_ASSERT_DT_INST_HAS_CHARMAP(n);                                                       \
                                                                                                   \
    static const struct behavior_send_string_config behavior_send_string_config_##n = {            \
        .text = DT_INST_PROP(n, text),                                                             \
        .config = ZMK_SEND_STRING_CONFIG_DT_INST_PROP(n),                                          \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_send_string_init, NULL, NULL,                              \
                            &behavior_send_string_config_##n, POST_KERNEL,                         \
                            CONFIG_APPLICATION_INIT_PRIORITY, &behavior_send_string_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SEND_STRING_INST);
