/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/devicetree.h>
#include <zmk/behavior.h>

#define ZMK_COMBOS_UTIL_ONE(n) +1

#define ZMK_COMBOS_FOREACH(_fn)                                                                    \
    COND_CODE_1(IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME),                                             \
                (DT_FOREACH_CHILD(DT_INST(0, zmk_combos), _fn)),                                   \
                (DT_FOREACH_CHILD_STATUS_OKAY(DT_INST(0, zmk_combos), _fn)))

#define ZMK_COMBOS_FOREACH_SEP(_fn, _sep)                                                          \
    COND_CODE_1(IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME),                                             \
                (DT_FOREACH_CHILD_SEP(DT_INST(0, zmk_combos), _fn, _sep)),                         \
                (DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_INST(0, zmk_combos), _fn, _sep)))

#define ZMK_STATIC_COMBOS_LEN                                                                      \
    COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(zmk_combos),                                             \
                (0 DT_FOREACH_CHILD_STATUS_OKAY(DT_INST(0, zmk_combos), ZMK_COMBOS_UTIL_ONE)),     \
                (0))

#define ZMK_COMBOS_LEN                                                                             \
    COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(zmk_combos),                                             \
                (0 ZMK_COMBOS_FOREACH(ZMK_COMBOS_UTIL_ONE)), (0))

#define COMBOS_KEYS_BYTE_ARRAY(node_id)                                                            \
    uint8_t _CONCAT(combo_prop_, node_id)[DT_PROP_LEN_OR(node_id, key_positions, 0)];

#define MAX_COMBO_KEYS                                                                             \
    COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(zmk_combos),                                             \
                (sizeof(union {ZMK_COMBOS_FOREACH(COMBOS_KEYS_BYTE_ARRAY)})), (0))

struct combo_cfg {
    uint16_t key_positions[MAX_COMBO_KEYS];
    int16_t key_position_len;
    int16_t require_prior_idle_ms;
    int32_t timeout_ms;
    uint32_t layer_mask;
    struct zmk_behavior_binding behavior;
    // if slow release is set, the combo releases when the last key is released.
    // otherwise, the combo releases when the first key is released.
    bool slow_release;
};

#if IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)

typedef int zmk_combo_runtime_id_t;

struct zmk_combo_runtime {
    zmk_combo_runtime_id_t id;
    struct combo_cfg combo;
};

bool zmk_combos_check_unsaved_changes(void);
int zmk_combos_reset_settings(void);
int zmk_combos_save_changes(void);
int zmk_combos_discard_changes(void);
// TODO: Document
// Returns non-negative combo ID value on success
// Returns negative errno on error
int zmk_combo_runtime_add_combo(const struct combo_cfg *cfg);
int zmk_combo_runtime_remove_combo(zmk_combo_runtime_id_t combo_id);

int zmk_combo_runtime_set_combo_binding(zmk_combo_runtime_id_t combo_id,
                                        const struct zmk_behavior_binding *binding);
int zmk_combo_runtime_add_combo_position(zmk_combo_runtime_id_t combo_id, uint16_t position);
int zmk_combo_runtime_remove_combo_position(zmk_combo_runtime_id_t combo_id, uint16_t position);

int zmk_combo_runtime_clear_combo_layers(zmk_combo_runtime_id_t combo_id);
int zmk_combo_runtime_set_combo_layer(zmk_combo_runtime_id_t combo_id, uint8_t layer, bool enabled);

int zmk_combo_runtime_set_combo_timeout(zmk_combo_runtime_id_t combo_id, uint16_t timeout);
int zmk_combo_runtime_set_combo_prior_idle(zmk_combo_runtime_id_t combo_id, uint16_t prior_idle);
int zmk_combo_runtime_set_combo_slow_release(zmk_combo_runtime_id_t combo_id, bool enabled);

int zmk_combo_runtime_get_combos(const struct zmk_combo_runtime **list);
const struct zmk_combo_runtime *zmk_combo_runtime_get_combo(zmk_combo_runtime_id_t combo_id);

int zmk_combo_runtime_get_free_combos(void);

#endif
