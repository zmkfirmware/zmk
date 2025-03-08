/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#include <zmk/split/bluetooth/peripheral_layers.h>
#include <zmk/keymap.h>

static uint32_t peripheral_layers = 0;

void set_peripheral_layers_state(uint32_t new_layers) { peripheral_layers = new_layers; }

bool peripheral_layer_active(uint8_t layer) {
    return (peripheral_layers & (BIT(layer))) == (BIT(layer));
};

uint8_t peripheral_highest_layer_active(void) {
    if (peripheral_layers > 0) {
        for (uint8_t layer = ZMK_KEYMAP_LAYERS_LEN - 1; layer > 0; layer--) {
            if ((peripheral_layers & (BIT(layer))) == (BIT(layer)) || layer == 0) {
                return layer;
            }
        }
    }
    return 0;
}