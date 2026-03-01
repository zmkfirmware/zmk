/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <zmk/keymap.h>

#define ZMK_RGB_CHILD_LEN_PLUS_ONE(node) 1 +

#define ZMK_RGBMAP_LAYERS_LEN                                                                      \
    (DT_FOREACH_CHILD(DT_INST(0, zmk_underglow_layer), ZMK_RGB_CHILD_LEN_PLUS_ONE) 0)

#define ZMK_RGBMAP_EXTRACT_BINDING(idx, drv_inst)                                                  \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(drv_inst, bindings, idx)),                \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(drv_inst, bindings, idx, param1), (0),        \
                              (DT_PHA_BY_IDX(drv_inst, bindings, idx, param1))),                   \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(drv_inst, bindings, idx, param2), (0),        \
                              (DT_PHA_BY_IDX(drv_inst, bindings, idx, param2))),                   \
    }

const int rgb_pixel_lookup(int idx);
const int zmk_rgbmap_id(uint8_t layer);
const int zmk_rgbmap_fade_delay(uint8_t layer);

const struct zmk_behavior_binding *rgb_underglow_get_bindings(uint8_t layer);

uint8_t rgb_underglow_top_layer_with_state(uint32_t state_to_test);
uint8_t rgb_underglow_top_layer(void);