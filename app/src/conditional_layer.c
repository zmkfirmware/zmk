/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_conditional_layers

#include <stdint.h>

#include <devicetree.h>
#include <logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/keymap.h>
#include <zmk/events/layer_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// Conditional layer configuration that activates the specified then-layer when all if-layers are
// active. With two if-layers, this is referred to as "tri-layer", and is commonly used to activate
// a third "adjust" layer if and only if the "lower" and "raise" layers are both active.
struct conditional_layer_cfg {
    // A bitmask of each layer that must be pressed for this conditional layer config to activate.
    zmk_keymap_layers_state_t if_layers_state_mask;

    // The layer number that should be active while all layers in the if-layers mask are active.
    int8_t then_layer;
};

#define IF_LAYER_BIT(i, n) BIT(DT_PROP_BY_IDX(n, if_layers, i)) |

// Evaluates to conditional_layer_cfg struct initializer.
#define CONDITIONAL_LAYER_DECL(n)                                                                  \
    {                                                                                              \
        /* TODO: Replace UTIL_LISTIFY with DT_FOREACH_PROP_ELEM after Zepyhr 2.6.0 upgrade. */     \
        .if_layers_state_mask = UTIL_LISTIFY(DT_PROP_LEN(n, if_layers), IF_LAYER_BIT, n) 0,        \
        .then_layer = DT_PROP(n, then_layer),                                                      \
    },

// All conditional layer configurations in the keymap.
static const struct conditional_layer_cfg CONDITIONAL_LAYER_CFGS[] = {
    DT_INST_FOREACH_CHILD(0, CONDITIONAL_LAYER_DECL)};

static const int32_t NUM_CONDITIONAL_LAYER_CFGS =
    sizeof(CONDITIONAL_LAYER_CFGS) / sizeof(*CONDITIONAL_LAYER_CFGS);

static void conditional_layer_activate(int8_t layer) {
    // This may trigger another event that could, in turn, activate additional then-layers. However,
    // the process will eventually terminate (at worst, when every layer is active).
    if (!zmk_keymap_layer_active(layer)) {
        LOG_DBG("layer %d", layer);
        zmk_keymap_layer_activate(layer);
    }
}

static void conditional_layer_deactivate(int8_t layer) {
    // This may deactivate a then-layer that's already active via another mechanism (e.g., a
    // momentary layer behavior). However, the same problem arises when multiple keys with the same
    // &mo binding are held and then one is released, so it's probably not an issue in practice.
    if (zmk_keymap_layer_active(layer)) {
        LOG_DBG("layer %d", layer);
        zmk_keymap_layer_deactivate(layer);
    }
}

// On layer state changes, examines each conditional layer config to determine if then-layer in the
// config should activate based on the currently active set of if-layers.
static int layer_state_changed_listener(const zmk_event_t *ev) {
    for (int i = 0; i < NUM_CONDITIONAL_LAYER_CFGS; i++) {
        const struct conditional_layer_cfg *cfg = CONDITIONAL_LAYER_CFGS + i;
        zmk_keymap_layers_state_t mask = cfg->if_layers_state_mask;

        // Activate then-layer if and only if all if-layers are already active. Note that we
        // reevaluate the current layer state for each config since activation of one layer can also
        // trigger activation of another.
        if ((zmk_keymap_layer_state() & mask) == mask) {
            conditional_layer_activate(cfg->then_layer);
        } else {
            conditional_layer_deactivate(cfg->then_layer);
        }
    }
    return 0;
}

ZMK_LISTENER(conditional_layer, layer_state_changed_listener);
ZMK_SUBSCRIPTION(conditional_layer, zmk_layer_state_changed);

#endif
