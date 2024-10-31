/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/events/position_state_changed.h>

#define ZMK_LAYER_CHILD_LEN_PLUS_ONE(node) 1 +
#define ZMK_KEYMAP_LAYERS_LEN                                                                      \
    (DT_FOREACH_CHILD(DT_INST(0, zmk_keymap), ZMK_LAYER_CHILD_LEN_PLUS_ONE) 0)

/**
 * @brief A layer ID is a stable identifier to refer to a layer, regardless of ordering.
 */
typedef uint8_t zmk_keymap_layer_id_t;

/**
 * @brief A way to return/reference a missing or invalid layer ID
 */
#define ZMK_KEYMAP_LAYER_ID_INVAL UINT8_MAX

/**
 * @brief A layer index is a identifier to refer layer at a particular position in the order.
 */
typedef uint8_t zmk_keymap_layer_index_t;

typedef uint32_t zmk_keymap_layers_state_t;

zmk_keymap_layer_id_t zmk_keymap_layer_index_to_id(zmk_keymap_layer_index_t layer_index);

zmk_keymap_layer_id_t zmk_keymap_layer_default(void);
zmk_keymap_layers_state_t zmk_keymap_layer_state(void);
bool zmk_keymap_layer_active(zmk_keymap_layer_id_t layer);
zmk_keymap_layer_index_t zmk_keymap_highest_layer_active(void);
int zmk_keymap_layer_activate(zmk_keymap_layer_id_t layer);
int zmk_keymap_layer_deactivate(zmk_keymap_layer_id_t layer);
int zmk_keymap_layer_toggle(zmk_keymap_layer_id_t layer);
int zmk_keymap_layer_to(zmk_keymap_layer_id_t layer);
const char *zmk_keymap_layer_name(zmk_keymap_layer_id_t layer);

const struct zmk_behavior_binding *zmk_keymap_get_layer_binding_at_idx(zmk_keymap_layer_id_t layer,
                                                                       uint8_t binding_idx);
int zmk_keymap_set_layer_binding_at_idx(zmk_keymap_layer_id_t layer, uint8_t binding_idx,
                                        const struct zmk_behavior_binding binding);

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_LAYER_REORDERING)

int zmk_keymap_add_layer(void);
int zmk_keymap_remove_layer(zmk_keymap_layer_index_t index);
int zmk_keymap_restore_layer(zmk_keymap_layer_id_t id, zmk_keymap_layer_index_t at_index);

int zmk_keymap_move_layer(zmk_keymap_layer_index_t start_idx, zmk_keymap_layer_index_t dest_idx);

int zmk_keymap_set_layer_name(zmk_keymap_layer_id_t id, const char *name, size_t size);

#endif

/**
 * @brief Check if there are any unsaved keymap changes.
 *
 * @retval 0 if there are no changes.
 * @retval 1 if there are changes.
 */
int zmk_keymap_check_unsaved_changes(void);

int zmk_keymap_save_changes(void);
int zmk_keymap_discard_changes(void);
int zmk_keymap_reset_settings(void);

int zmk_keymap_position_state_changed(uint8_t source, uint32_t position, bool pressed,
                                      int64_t timestamp);

#define ZMK_KEYMAP_EXTRACT_BINDING(idx, drv_inst)                                                  \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(drv_inst, bindings, idx)),                \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(drv_inst, bindings, idx, param1), (0),        \
                              (DT_PHA_BY_IDX(drv_inst, bindings, idx, param1))),                   \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(drv_inst, bindings, idx, param2), (0),        \
                              (DT_PHA_BY_IDX(drv_inst, bindings, idx, param2))),                   \
    }
