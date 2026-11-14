/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_input_speed

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <drivers/input_processor_speed_profile.h>

#include <dt-bindings/zmk/input_speed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_input_speed_config {
    const struct device *processor;
};

static int behavior_input_speed_calculate_level(const struct device *processor, int8_t direction,
                                                uint8_t *level) {
    uint8_t current;
    uint8_t count;
    int err = zmk_input_processor_speed_profile_get_level(processor, &current);

    if (err != 0) {
        return err;
    }

    err = zmk_input_processor_speed_profile_get_level_count(processor, &count);
    if (err != 0) {
        return err;
    }

    int16_t target = CLAMP((int16_t)current + direction, 1, count);
    *level = (uint8_t)target;
    return 0;
}

static int
on_keymap_binding_convert_central_state_dependent_params(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    const struct device *behavior_dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_input_speed_config *config = behavior_dev->config;
    int8_t direction;

    ARG_UNUSED(event);

    switch (binding->param1) {
    case INPUT_SPEED_DOWN_CMD:
        direction = -1;
        break;
    case INPUT_SPEED_UP_CMD:
        direction = 1;
        break;
    default:
        return 0;
    }

    uint8_t level;
    int err = behavior_input_speed_calculate_level(config->processor, direction, &level);
    if (err != 0) {
        return err;
    }

    binding->param1 = INPUT_SPEED_SET_CMD;
    binding->param2 = level;
    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *behavior_dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_input_speed_config *config = behavior_dev->config;
    uint8_t level;
    int err;

    ARG_UNUSED(event);

    switch (binding->param1) {
    case INPUT_SPEED_DOWN_CMD:
        err = behavior_input_speed_calculate_level(config->processor, -1, &level);
        break;
    case INPUT_SPEED_UP_CMD:
        err = behavior_input_speed_calculate_level(config->processor, 1, &level);
        break;
    case INPUT_SPEED_SET_CMD:
        if (binding->param2 == 0U || binding->param2 > UINT8_MAX) {
            return -EINVAL;
        }
        level = (uint8_t)binding->param2;
        err = 0;
        break;
    default:
        LOG_ERR("Unknown input speed command: %d", binding->param1);
        return -ENOTSUP;
    }

    if (err != 0) {
        return err;
    }

    return zmk_input_processor_speed_profile_set_level(config->processor, level);
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_input_speed_init(const struct device *dev) {
    const struct behavior_input_speed_config *config = dev->config;

    if (!device_is_ready(config->processor)) {
        LOG_ERR("Input speed processor %s is not ready", config->processor->name);
        return -ENODEV;
    }

    uint8_t count;
    return zmk_input_processor_speed_profile_get_level_count(config->processor, &count);
}

static const struct behavior_driver_api behavior_input_speed_driver_api = {
    .binding_convert_central_state_dependent_params =
        on_keymap_binding_convert_central_state_dependent_params,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
};

#define BEHAVIOR_INPUT_SPEED_DEFINE(inst)                                                          \
    BUILD_ASSERT(                                                                                  \
        DT_NODE_HAS_COMPAT(DT_INST_PHANDLE(inst, processor), zmk_input_processor_speed_profile),   \
        "processor must reference a zmk,input-processor-speed-profile node");                      \
    static const struct behavior_input_speed_config behavior_input_speed_config_##inst = {         \
        .processor = DEVICE_DT_GET(DT_INST_PHANDLE(inst, processor)),                              \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(                                                                       \
        inst, behavior_input_speed_init, NULL, NULL, &behavior_input_speed_config_##inst,          \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_input_speed_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BEHAVIOR_INPUT_SPEED_DEFINE)
