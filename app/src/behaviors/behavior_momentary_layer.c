/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_momentary_layer

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/momentary_layer.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .display_name = "Layer",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_LAYER_INDEX,
    },
};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = param_values,
    .param1_values_len = ARRAY_SIZE(param_values),
}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};

#endif

struct behavior_mo_config {};
struct behavior_mo_data {
    zmk_keymap_layers_state_t active_momentary_layers;
    zmk_keymap_layers_state_t ignore_on_release;
};

static const struct behavior_mo_config behavior_mo_config = {};
static struct behavior_mo_data behavior_mo_data;

zmk_keymap_layers_state_t zmk_lock_active_momentary_layers() {
    return behavior_mo_data.ignore_on_release = behavior_mo_data.active_momentary_layers;
}

static int behavior_mo_init(const struct device *dev) { return 0; };

static int mo_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    int layer = binding->param1;
    LOG_DBG("position %d layer %d", event.position, layer);
    WRITE_BIT(behavior_mo_data.active_momentary_layers, layer, true);
    return zmk_keymap_layer_activate(layer);
}

static int mo_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    int layer = binding->param1;
    LOG_DBG("position %d layer %d", event.position, layer);
    WRITE_BIT(behavior_mo_data.active_momentary_layers, layer, false);

    // If the layer is locked, don't deactivate it. Instead clear the
    // ignore_on_release flag so the next press/release will.
    if (behavior_mo_data.ignore_on_release & BIT(layer)) {
        WRITE_BIT(behavior_mo_data.ignore_on_release, layer, false);
        return 0;
    }

    return zmk_keymap_layer_deactivate(binding->param1);
}

static const struct behavior_driver_api behavior_mo_driver_api = {
    .binding_pressed = mo_keymap_binding_pressed,
    .binding_released = mo_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_mo_init, NULL, &behavior_mo_data, &behavior_mo_config,
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_mo_driver_api);