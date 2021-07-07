/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_multi_layers

#include <stdint.h>

#include <devicetree.h>
#include <logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/keymap.h>
#include <zmk/events/layer_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// A single multi-layer configuration from the keymap. With two "if-layers", this is referred to as
// "tri-layer", and is commonly used to activate a third "adjust" layer if and only if the "lower"
// and "raise" layers are both active.
struct multi_layer_cfg {
    // A bitmask of each layer that must be pressed for this multi-layer config to activate.
    zmk_keymap_layers_state_t if_layers_state_mask;

    // The layer number that should be active while all layers in the if-layers mask are active.
    int8_t then_layer;
};

#define IF_LAYER_BIT(i, n) BIT(DT_PROP_BY_IDX(n, if_layers, i)) |

// Evaluates to multi_layer_cfg struct initializer.
#define MULTI_LAYER_DECL(n)                                                                        \
    {                                                                                              \
        .if_layers_state_mask = UTIL_LISTIFY(DT_PROP_LEN(n, if_layers), IF_LAYER_BIT, n) 0,        \
        .then_layer = DT_PROP(n, then_layer),                                                      \
    },

// All the multi-layer configuration entries from the keymap.
static struct multi_layer_cfg multi_layer_cfgs[] = {DT_INST_FOREACH_CHILD(0, MULTI_LAYER_DECL)};

static const int32_t NUM_MULTI_LAYER_CFGS =
    sizeof(multi_layer_cfgs) / sizeof(struct multi_layer_cfg);

static void multi_layer_activate(int8_t layer) {
    // TODO(bcat): This may trigger another event that could, in turn, activate additional
    // then-layers. The process must eventually terminate (at worst, when every layer is
    // active), but we should either intentionally document or explicitly disallow this.
    if (!zmk_keymap_layer_active(layer)) {
        LOG_DBG("layer %d", layer);
        zmk_keymap_layer_activate(layer);
    }
}

static void multi_layer_deactivate(int8_t layer) {
    // TODO(bcat): This may deactivate a then-layer that's already active via another
    // mechanism (e.g. a momentary layer behavior). We should either declare that combining
    // multi-layer support with other layer activation mechanisms yields undefined behavior,
    // or else we should implement a separate bitset for multi-layer activations that's OR'd
    // with the existing bitset to obtain the "effective layers state" at any given time.
    if (zmk_keymap_layer_active(layer)) {
        LOG_DBG("layer %d", layer);
        zmk_keymap_layer_deactivate(layer);
    }
}

// On layer state changes, examines each multi-layer config to determine if then-layer in the config
// should activate based on the currently active set of if-layers.
static int layer_state_changed_listener(const zmk_event_t *ev) {
    zmk_keymap_layers_state_t layers_state = zmk_keymap_layer_state();
    for (int i = 0; i < NUM_MULTI_LAYER_CFGS; i++) {
        const struct multi_layer_cfg *cfg = multi_layer_cfgs + i;
        zmk_keymap_layers_state_t mask = cfg->if_layers_state_mask;

        // Activate then-layer if and only if all if-layers are already active.
        if ((layers_state & mask) == mask) {
            multi_layer_activate(cfg->then_layer);
        } else {
            multi_layer_deactivate(cfg->then_layer);
        }
    }
    return 0;
}

ZMK_LISTENER(multi_layer, layer_state_changed_listener);
ZMK_SUBSCRIPTION(multi_layer, zmk_layer_state_changed);

#endif
