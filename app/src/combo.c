/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_combos

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>
#include <zmk/virtual_key_position.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME) && CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO > 0

#warning                                                                                           \
    "CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO is deprecated, and is auto-calculated from the devicetree now."

#endif

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME) && CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY > 0

#warning "CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY is deprecated, and is auto-calculated."

#endif

struct active_combo {
    uint16_t combo_idx;
    // key_positions_pressed is filled with key_positions when the combo is pressed.
    // The keys are removed from this array when they are released.
    // Once this array is empty, the behavior is released.
    uint16_t key_positions_pressed_count;
    struct zmk_position_state_changed_event key_positions_pressed[MAX_COMBO_KEYS];
};

#define PROP_BIT_AT_IDX(n, prop, idx) BIT(DT_PROP_BY_IDX(n, prop, idx))

#define NODE_PROP_BITMASK(n, prop)                                                                 \
    COND_CODE_1(DT_NODE_HAS_PROP(n, prop),                                                         \
                (DT_FOREACH_PROP_ELEM_SEP(n, prop, PROP_BIT_AT_IDX, (|))), (0))

#define GET_KEY_POSITION_MASK_PORTION(idx, n) ((NODE_PROP_BITMASK(n, key_positions) >> idx) & 0xFF)

#define COMBO_INST(n, positions)                                                                   \
    COND_CODE_1(IS_EQ(DT_PROP_LEN(n, key_positions), positions),                                   \
                (                                                                                  \
                    {                                                                              \
                        .key_positions = DT_PROP(n, key_positions),                                \
                        .key_position_len = DT_PROP_LEN(n, key_positions),                         \
                        .timeout_ms = DT_PROP(n, timeout_ms),                                      \
                        .require_prior_idle_ms = DT_PROP(n, require_prior_idle_ms),                \
                        .behavior = ZMK_KEYMAP_EXTRACT_BINDING(0, n),                              \
                        .slow_release = DT_PROP(n, slow_release),                                  \
                        .layer_mask = NODE_PROP_BITMASK(n, layers),                                \
                    }, ),                                                                          \
                ())

#define COMBO_CONFIGS_WITH_MATCHING_POSITIONS_LEN(positions, _ignore)                              \
    DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(0, COMBO_INST, positions)

// We do some magic here to generate the `combos` array by "key position length", looping
// by key position length and on each iteration, only include entries where the `key-positions`
// length matches.
// Doing so allows our bitmasks to be "sorted key positions list first" when searching for matches.
// `20` is chosen as a reasonable limit, since the theoretical maximum number of keys you might
// reasonably press simultaneously with 10 fingers is 20 keys, two keys per finger.
static const struct combo_cfg combos[] = {
    LISTIFY(20, COMBO_CONFIGS_WITH_MATCHING_POSITIONS_LEN, (), 0)};

static void reload_combo_lookup(void);
static int initialize_combo(size_t index);

#if IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)

struct combo_settings_storage {
    struct combo_settings_storage_core {
        int16_t require_prior_idle_ms;
        int32_t timeout_ms;
        uint32_t layer_mask;
        struct zmk_behavior_binding_setting behavior;
        uint8_t slow_release;
    } core;
    /* For settings storage, we store these at the end, to save a bit */
    uint16_t key_positions[MAX_COMBO_KEYS];
} __packed;

static struct zmk_combo_runtime runtime_combos[ZMK_COMBOS_LEN];

BUILD_ASSERT(ZMK_COMBOS_LEN <= 64, "A maximum of 64 combos is supported for runtime combos");

#if ZMK_COMBOS_LEN > 32
typedef uint64_t combos_runtime_change_mask_t;
#else
typedef uint32_t combos_runtime_change_mask_t;
#endif

static combos_runtime_change_mask_t runtime_combos_changed_ids;

int zmk_combo_runtime_get_combos(const struct zmk_combo_runtime **list) {
    *list = &runtime_combos[0];
    for (size_t i = 0; i < ZMK_COMBOS_LEN; i++) {
        if (runtime_combos[i].combo.key_position_len == 0) {
            return i;
        }
    }

    return ZMK_COMBOS_LEN;
}

static int compare_key_positions(const void *a, const void *b) {
    const int16_t *kp_a = a;
    const int16_t *kp_b = b;

    return (*kp_a) - (*kp_b);
}

static int compare_combos_by_kp_len_and_kp(const void *a, const void *b) {
    const struct zmk_combo_runtime *c_a = a;
    const struct zmk_combo_runtime *c_b = b;

    if (!c_a->combo.key_position_len) {
        return INT32_MAX;
    } else if (!c_b->combo.key_position_len) {
        return INT32_MIN;
    } else if (c_a->combo.key_position_len != c_b->combo.key_position_len) {
        return c_a->combo.key_position_len - c_b->combo.key_position_len;
    }

    for (size_t i = 0; i < c_a->combo.key_position_len; i++) {
        if (c_a->combo.key_positions[i] != c_b->combo.key_positions[i]) {
            return c_a->combo.key_positions[i] - c_b->combo.key_positions[i];
        }
    }

    return 0;
}

void reindex_combos(void) {
    qsort(&runtime_combos, ZMK_COMBOS_LEN, sizeof(runtime_combos[0]),
          compare_combos_by_kp_len_and_kp);

    reload_combo_lookup();
}

static void mark_combo_changed(zmk_combo_runtime_id_t combo_id) {
    WRITE_BIT(runtime_combos_changed_ids, combo_id, true);
}

static int find_runtime_idx(zmk_combo_runtime_id_t combo_id) {
    for (int i = 0; i < ARRAY_SIZE(runtime_combos); i++) {
        if (runtime_combos[i].id == combo_id) {
            return i;
        }
    }

    return -EINVAL;
}

static int add_position_to_runtime_combo(struct zmk_combo_runtime *rc, uint16_t position) {
    __ASSERT(rc != NULL, "Passed a NULL combo");

    if (rc->combo.key_position_len >= MAX_COMBO_KEYS) {
        return -ENOMEM;
    }

    // Return success if the position is already enabled;
    for (size_t i = 0; i < rc->combo.key_position_len; i++) {
        if (rc->combo.key_positions[i] == position) {
            return 0;
        }
    }

    rc->combo.key_positions[rc->combo.key_position_len++] = position;
    qsort(&rc->combo.key_positions, rc->combo.key_position_len, sizeof(rc->combo.key_positions[0]),
          compare_key_positions);

    mark_combo_changed(rc->id);
    reindex_combos();

    return 0;
}

int zmk_combo_runtime_add_combo(const struct combo_cfg *cfg) {
    for (int c = 0; c < ARRAY_SIZE(runtime_combos); c++) {
        if (runtime_combos[c].combo.key_position_len == 0) {
            memcpy(&runtime_combos[c].combo, cfg, sizeof(struct combo_cfg));

            zmk_combo_runtime_id_t id = runtime_combos[c].id;
            reindex_combos();

            mark_combo_changed(id);

            return id;
        }
    }

    return -ENOMEM;
}

static struct zmk_combo_runtime *get_combo_for_id(zmk_combo_runtime_id_t combo_id) {
    int combo_idx = find_runtime_idx(combo_id);

    if (combo_idx < 0 || combo_idx >= ARRAY_SIZE(runtime_combos)) {
        return NULL;
    }

    return &runtime_combos[combo_idx];
}
const struct zmk_combo_runtime *zmk_combo_runtime_get_combo(zmk_combo_runtime_id_t combo_id) {
    return get_combo_for_id(combo_id);
}

int zmk_combo_runtime_set_combo_binding(zmk_combo_runtime_id_t combo_id,
                                        const struct zmk_behavior_binding *binding) {
    struct zmk_combo_runtime *rc = get_combo_for_id(combo_id);
    if (!rc) {
        return -EINVAL;
    }

    rc->combo.behavior = *binding;
    mark_combo_changed(rc->id);
    return 0;
}

int zmk_combo_runtime_add_combo_position(zmk_combo_runtime_id_t combo_id, uint16_t position) {
    int ret;
    struct zmk_combo_runtime *rc = get_combo_for_id(combo_id);
    if (!rc) {
        return -EINVAL;
    }

    ret = add_position_to_runtime_combo(rc, position);
    if (ret < 0) {
        return ret;
    }

    mark_combo_changed(rc->id);

    return 0;
}

int zmk_combo_runtime_remove_combo_position(zmk_combo_runtime_id_t combo_id, uint16_t position) {
    ;
    struct zmk_combo_runtime *rc = get_combo_for_id(combo_id);
    if (!rc) {
        return -EINVAL;
    }

    for (size_t i = 0; i < rc->combo.key_position_len; i++) {
        if (rc->combo.key_positions[i] == position) {
            size_t to_move = rc->combo.key_position_len - i - 1;
            if (to_move > 0) {
                memmove(&rc->combo.key_positions[i], &rc->combo.key_positions[i + 1],
                        to_move * sizeof(rc->combo.key_positions[0]));
            }

            rc->combo.key_position_len--;

            reindex_combos();
            mark_combo_changed(rc->id);

            return 0;
        }
    }

    return -ENODEV;
}

int zmk_combo_runtime_set_combo_layer(zmk_combo_runtime_id_t combo_id, uint8_t layer,
                                      bool enabled) {
    struct zmk_combo_runtime *rc = get_combo_for_id(combo_id);
    if (!rc) {
        return -EINVAL;
    }

    if (rc->combo.key_position_len == 0) {
        return -EINVAL;
    }

    WRITE_BIT(rc->combo.layer_mask, layer, enabled);
    mark_combo_changed(rc->id);
    return 0;
}

int zmk_combo_runtime_clear_combo_layers(zmk_combo_runtime_id_t combo_id) {
    struct zmk_combo_runtime *rc = get_combo_for_id(combo_id);
    if (!rc) {
        return -EINVAL;
    }

    if (rc->combo.key_position_len == 0) {
        return -EINVAL;
    }

    rc->combo.layer_mask = 0;
    mark_combo_changed(rc->id);

    return 0;
}

int zmk_combo_runtime_set_combo_timeout(zmk_combo_runtime_id_t combo_id, uint16_t timeout) {
    struct zmk_combo_runtime *rc = get_combo_for_id(combo_id);
    if (!rc) {
        return -EINVAL;
    }

    if (timeout <= 0) {
        return -EINVAL;
    }

    if (rc->combo.key_position_len == 0) {
        return -EINVAL;
    }

    rc->combo.timeout_ms = timeout;
    mark_combo_changed(rc->id);

    return 0;
}

int zmk_combo_runtime_set_combo_prior_idle(zmk_combo_runtime_id_t combo_id, uint16_t prior_idle) {
    struct zmk_combo_runtime *rc = get_combo_for_id(combo_id);
    if (!rc) {
        return -EINVAL;
    }

    if (prior_idle < 0) {
        return -EINVAL;
    }

    if (rc->combo.key_position_len == 0) {
        return -EINVAL;
    }

    rc->combo.require_prior_idle_ms = prior_idle;
    mark_combo_changed(rc->id);

    return 0;
}

int zmk_combo_runtime_set_combo_slow_release(zmk_combo_runtime_id_t combo_id, bool enabled) {
    struct zmk_combo_runtime *rc = get_combo_for_id(combo_id);
    if (!rc) {
        return -EINVAL;
    }

    if (rc->combo.key_position_len == 0) {
        return -EINVAL;
    }

    rc->combo.slow_release = enabled;
    mark_combo_changed(rc->id);

    return 0;
}

int zmk_combo_runtime_remove_combo(zmk_combo_runtime_id_t combo_id) {
    int combo_idx = find_runtime_idx(combo_id);
    LOG_DBG("Removing %d at %d", combo_id, combo_idx);

    if (combo_idx < 0 || combo_idx >= ARRAY_SIZE(runtime_combos)) {
        return -EINVAL;
    }

    if (runtime_combos[combo_idx].combo.key_position_len == 0) {
        return -EINVAL;
    }

    if (combo_idx == ZMK_COMBOS_LEN - 1) {
        memset(&runtime_combos[combo_idx].combo, 0, sizeof(struct combo_cfg));
        LOG_DBG("index %d has id %d", combo_idx, runtime_combos[combo_idx].id);

    } else {
        for (size_t i = ZMK_COMBOS_LEN - 1; i >= combo_id; i--) {
            if (runtime_combos[i].combo.key_position_len > 0) {
                memmove(&runtime_combos[combo_idx], &runtime_combos[combo_idx + 1],
                        (i - combo_idx) * sizeof(runtime_combos[0]));
                memset(&runtime_combos[i].combo, 0, sizeof(struct combo_cfg));
                /* Ensure the removed ID isn't dropped, just placed at the end of the list */
                runtime_combos[i].id = combo_id;
                break;
            }
        }
    }

    LOG_DBG("index %d has id %d", combo_idx, runtime_combos[combo_idx].id);
    reindex_combos();
    LOG_DBG("index %d has id %d", combo_idx, runtime_combos[combo_idx].id);
    LOG_DBG("Marking %d as changed", combo_id);
    mark_combo_changed(combo_id);

    return 0;
}

int zmk_combo_runtime_get_free_combos(void) {
    int ret = 0;

    for (int i = ARRAY_SIZE(runtime_combos) - 1; i >= 0; i--) {
        if (runtime_combos[i].combo.key_position_len > 0) {
            break;
        }

        ret++;
    }

    return ret;
}

static void reload_from_static(void) {
    memset(runtime_combos, 0, ARRAY_SIZE(runtime_combos) * sizeof(struct zmk_combo_runtime));
    for (int i = 0; i < ARRAY_SIZE(runtime_combos); i++) {
        runtime_combos[i].id = i;
        if (i < ARRAY_SIZE(combos)) {
            memcpy(&runtime_combos[i].combo, &combos[i], sizeof(struct combo_cfg));
        }
    }

    reload_combo_lookup();
}

bool zmk_combos_check_unsaved_changes(void) { return runtime_combos_changed_ids != 0; }

#define COMBOS_SETTING_NAME_PREFIX "zmk/combos"

int zmk_combos_reset_settings(void) {
#if IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME_SETTINGS_STORAGE)
    // Delete the settings saved for eevery combo ID
    for (int i = 0; i < ZMK_COMBOS_LEN; i++) {
        char setting_name[14];
        struct zmk_combo_runtime *rc = &runtime_combos[i];
        sprintf(setting_name, COMBOS_SETTING_NAME_PREFIX "/%d", rc->id);
        settings_delete(setting_name);
    }

#endif // IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME_SETTINGS_STORAGE)

    reload_from_static();

    return 0;
}

#if IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME_SETTINGS_STORAGE)

int save_runtime_combo(uint8_t combo_idx) {
    char setting_name[14];
    struct zmk_combo_runtime *rc = &runtime_combos[combo_idx];
    struct combo_settings_storage combo_storage = {
        .core =
            {
                .require_prior_idle_ms = rc->combo.require_prior_idle_ms,
                .timeout_ms = rc->combo.timeout_ms,
                .layer_mask = rc->combo.layer_mask,
                .behavior =
                    {
                        .behavior_local_id =
                            zmk_behavior_get_local_id(rc->combo.behavior.behavior_dev),
                        .param1 = rc->combo.behavior.param1,
                        .param2 = rc->combo.behavior.param2,
                    },
                .slow_release = rc->combo.slow_release,
            },
    };

    memcpy(combo_storage.key_positions, rc->combo.key_positions,
           MAX_COMBO_KEYS * sizeof(rc->combo.key_positions[0]));

    sprintf(setting_name, COMBOS_SETTING_NAME_PREFIX "/%d", rc->id);

    // Optimize storage a bit by only storing keys that are set
    return settings_save_one(setting_name, &combo_storage,
                             sizeof(struct combo_settings_storage_core) +
                                 rc->combo.key_position_len * sizeof(rc->combo.key_positions[0]));
}

#endif

int zmk_combos_save_changes(void) {
#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME_SETTINGS_STORAGE)
    return -ENOTSUP;
#else
    LOG_DBG("Changed? %d", runtime_combos_changed_ids);
    for (size_t i = 0; i < ZMK_COMBOS_LEN; i++) {
        struct zmk_combo_runtime *rc = &runtime_combos[i];
        LOG_DBG("Checking ID %d", rc->id);
        if (!IS_BIT_SET(runtime_combos_changed_ids, rc->id)) {
            continue;
        }

        LOG_DBG("Saving combo with ID %d", rc->id);

        int ret = save_runtime_combo(i);
        if (ret < 0) {
            LOG_DBG("Saving combo with id %d failed (%d)", rc->id, ret);
            return ret;
        }

        WRITE_BIT(runtime_combos_changed_ids, rc->id, false);
    }

    return 0;
#endif
}

int zmk_combos_discard_changes(void) {
    int ret = 0;

    reload_from_static();
#if IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME_SETTINGS_STORAGE)
    ret = settings_load_subtree(COMBOS_SETTING_NAME_PREFIX);
    if (ret < 0) {
        LOG_ERR("Failed to load a subtree %d", ret);
        return ret;
    }
#endif

    return ret;
}

static inline const struct combo_cfg *get_combo_with_id(int combo_id) {
    return &runtime_combos[combo_id].combo;
}

static int combos_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    // Load the combos from the settings
    //
    int ret;
    const char *rem;

    size_t name_len = settings_name_next(name, &rem);

    /* We shouldn't have anything other than the ID for the combo settings */
    if (rem || !name_len) {
        return -EINVAL;
    }

    char *endptr;
    zmk_combo_runtime_id_t combo_id = strtoul(name, &endptr, 10);
    if (*endptr != '\0') {
        LOG_ERR("Invalid combo ID %s", name);
        return -EINVAL;
    }

    struct combo_settings_storage stored_combo;
    if (len < sizeof(struct combo_settings_storage_core) || len > sizeof(stored_combo)) {
        LOG_ERR("Invalid stored combo size of %d", len);
        return -EINVAL;
    }

    ret = read_cb(cb_arg, &stored_combo, len);
    if (ret < 0) {
        LOG_ERR("Failed to load combo from settings (%d)", ret);
        return ret;
    }

    int idx = find_runtime_idx(combo_id);
    if (idx < 0) {
        LOG_ERR("Invalid combo ID %d", combo_id);
        return -ENODEV;
    }

    struct zmk_combo_runtime *rc = &runtime_combos[idx];

    rc->combo.behavior.param1 = stored_combo.core.behavior.param1;
    rc->combo.behavior.param2 = stored_combo.core.behavior.param2;
    rc->combo.behavior.local_id = stored_combo.core.behavior.behavior_local_id;
    rc->combo.slow_release = stored_combo.core.slow_release != 0;
    rc->combo.require_prior_idle_ms = stored_combo.core.require_prior_idle_ms;
    rc->combo.timeout_ms = stored_combo.core.timeout_ms;

    size_t num_of_positions =
        (len - sizeof(struct combo_settings_storage_core)) / sizeof(stored_combo.key_positions[0]);
    memcpy(rc->combo.key_positions, stored_combo.key_positions,
           num_of_positions * sizeof(stored_combo.key_positions[0]));
    rc->combo.key_position_len = num_of_positions;

    return 0;
}

static int combos_handle_commit(void) {
    for (int i = 0; i < ZMK_COMBOS_LEN; i++) {
        struct zmk_combo_runtime *rc = &runtime_combos[i];

        if (rc->combo.key_position_len && rc->combo.behavior.local_id > 0 &&
            !rc->combo.behavior.behavior_dev) {
            rc->combo.behavior.behavior_dev =
                zmk_behavior_find_behavior_name_from_local_id(rc->combo.behavior.local_id);

            if (!rc->combo.behavior.behavior_dev) {
                LOG_ERR("Failed to finding device for local ID %d after settings load",
                        rc->combo.behavior.local_id);
                return -EINVAL;
            }
        }
    }

    reindex_combos();
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(combos, COMBOS_SETTING_NAME_PREFIX, NULL, combos_handle_set,
                               combos_handle_commit, NULL);

#else

static inline const struct combo_cfg *get_combo_with_id(int combo_id) { return &combos[combo_id]; }

#endif

#define COMBO_ONE(n) +1

#define COMBO_CHILDREN_COUNT (0 DT_INST_FOREACH_CHILD(0, COMBO_ONE))

// We need at least 4 bytes to avoid alignment issues
#define BYTES_FOR_COMBOS_MASK DIV_ROUND_UP(COMBO_CHILDREN_COUNT, 32)

uint8_t pressed_keys_count = 0;
// set of keys pressed
struct zmk_position_state_changed_event pressed_keys[MAX_COMBO_KEYS] = {};
// the set of candidate combos based on the currently pressed_keys
uint32_t candidates[BYTES_FOR_COMBOS_MASK];
// the last candidate that was completely pressed
int16_t fully_pressed_combo = INT16_MAX;
// a lookup dict that maps a key position to all combos on that position
uint32_t combo_lookup[ZMK_KEYMAP_LEN][BYTES_FOR_COMBOS_MASK] = {};
// combos that have been activated and still have (some) keys pressed
// this array is always contiguous from 0.
struct active_combo active_combos[CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS] = {};
uint8_t active_combo_count = 0;

struct k_work_delayable timeout_task;
int64_t timeout_task_timeout_at;

// this keeps track of the last non-combo, non-mod key tap
int64_t last_tapped_timestamp = INT32_MIN;
// this keeps track of the last time a combo was pressed
int64_t last_combo_timestamp = INT32_MIN;

static void store_last_tapped(int64_t timestamp) {
    if (timestamp > last_combo_timestamp) {
        last_tapped_timestamp = timestamp;
    }
}

// Store the combo key pointer in the combos array, one pointer for each key position
// The combos are sorted shortest-first, then by virtual-key-position.
static int initialize_combo(size_t index) {
    const struct combo_cfg *new_combo = get_combo_with_id(index);

    for (size_t kp = 0; kp < new_combo->key_position_len; kp++) {
        sys_bitfield_set_bit((mem_addr_t)&combo_lookup[new_combo->key_positions[kp]], index);
    }

    return 0;
}

static void reload_combo_lookup(void) {
    memset(combo_lookup, 0, ZMK_KEYMAP_LEN * BYTES_FOR_COMBOS_MASK * sizeof(uint32_t));
    for (size_t i = 0; i < ZMK_COMBOS_LEN && get_combo_with_id(i)->key_position_len > 0; i++) {
        initialize_combo(i);
    }
}

static bool combo_active_on_layer(const struct combo_cfg *combo, uint8_t layer) {
    if (!combo->layer_mask) {
        return true;
    }

    return combo->layer_mask & BIT(layer);
}

static bool is_quick_tap(const struct combo_cfg *combo, int64_t timestamp) {
    return (last_tapped_timestamp + combo->require_prior_idle_ms) > timestamp;
}

static int setup_candidates_for_first_keypress(int32_t position, int64_t timestamp) {
    int number_of_combo_candidates = 0;
    uint8_t highest_active_layer = zmk_keymap_highest_layer_active();

    for (size_t i = 0; i < ZMK_COMBOS_LEN; i++) {
        const struct combo_cfg *combo = get_combo_with_id(i);
        if (combo->key_position_len > 1 &&
            sys_bitfield_test_bit((mem_addr_t)&combo_lookup[position], i)) {
            LOG_WRN("Git a matching position at index ");
            if (combo_active_on_layer(combo, highest_active_layer) &&
                !is_quick_tap(combo, timestamp)) {
                sys_bitfield_set_bit((mem_addr_t)&candidates, i);
                number_of_combo_candidates++;
            }
            // LOG_DBG("combo timeout %d %d %d", position, i, candidates[i].timeout_at);
        }
    }

    return number_of_combo_candidates;
}

static inline uint8_t zero_one_or_more_bits(uint32_t field) {
    if (field == 0) {
        return 0;
    }
    if ((field & (field - 1)) == 0) {
        return 1;
    }
    return 2;
}

static int filter_candidates(int32_t position) {
    int matches = 0;
    for (int i = 0; i < BYTES_FOR_COMBOS_MASK; i++) {
        candidates[i] &= combo_lookup[position][i];
        if (matches < 2) {
            matches += zero_one_or_more_bits(candidates[i]);
        }
    }

    LOG_DBG("combo matches after filter %d", matches);
    return matches;
}

static int64_t first_candidate_timeout() {
    if (pressed_keys_count == 0) {
        return LONG_MAX;
    }

    int64_t first_timeout = LONG_MAX;
    for (int i = 0; i < ZMK_COMBOS_LEN; i++) {
        const struct combo_cfg *combo = get_combo_with_id(i);
        if (combo->key_position_len == 0) {
            break;
        }

        if (sys_bitfield_test_bit((mem_addr_t)&candidates, i)) {
            first_timeout = MIN(first_timeout, combo->timeout_ms);
        }
    }

    return pressed_keys[0].data.timestamp + first_timeout;
}

static inline bool candidate_is_completely_pressed(const struct combo_cfg *candidate) {
    // this code assumes set(pressed_keys) <= set(candidate->key_positions)
    // this invariant is enforced by filter_candidates
    // since events may have been reraised after clearing one or more slots at
    // the start of pressed_keys (see: release_pressed_keys), we have to check
    // that each key needed to trigger the combo was pressed, not just the last.
    return candidate->key_position_len == pressed_keys_count;
}

static int cleanup();

static int filter_timed_out_candidates(int64_t timestamp) {
    __ASSERT(pressed_keys_count > 0, "Searching for a candidate timeout with no keys pressed");

    LOG_WRN("FILTER TIMED OUT!");
    int remaining_candidates = 0;
    for (int i = 0; i < ZMK_COMBOS_LEN; i++) {
        if (sys_bitfield_test_bit((mem_addr_t)&candidates, i)) {

            if (pressed_keys[0].data.timestamp + get_combo_with_id(i)->timeout_ms > timestamp) {
                remaining_candidates++;
            } else {
                sys_bitfield_clear_bit((mem_addr_t)&candidates, i);
            }
        }
    }

    LOG_DBG(
        "after filtering out timed out combo candidates: remaining_candidates=%d timestamp=%lld",
        remaining_candidates, timestamp);

    return remaining_candidates;
}

static int capture_pressed_key(const struct zmk_position_state_changed *ev) {
    if (pressed_keys_count == MAX_COMBO_KEYS) {
        LOG_WRN("Bubbling!");
        return ZMK_EV_EVENT_BUBBLE;
    }

    pressed_keys[pressed_keys_count++] = copy_raised_zmk_position_state_changed(ev);
    LOG_WRN("Captured they key!");
    return ZMK_EV_EVENT_CAPTURED;
}

const struct zmk_listener zmk_listener_combo;

static int release_pressed_keys() {
    uint8_t count = pressed_keys_count;
    pressed_keys_count = 0;
    for (int i = 0; i < count; i++) {
        struct zmk_position_state_changed_event *ev = &pressed_keys[i];
        if (i == 0) {
            LOG_DBG("combo: releasing position event %d", ev->data.position);
            ZMK_EVENT_RELEASE(*ev);
        } else {
            // reprocess events (see tests/combo/fully-overlapping-combos-3 for why this is needed)
            LOG_DBG("combo: reraising position event %d", ev->data.position);
            ZMK_EVENT_RAISE(*ev);
        }
    }

    return count;
}

static inline int press_combo_behavior(int combo_idx, const struct combo_cfg *combo,
                                       int32_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = ZMK_VIRTUAL_KEY_POSITION_COMBO(combo_idx),
        .timestamp = timestamp,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };

    last_combo_timestamp = timestamp;

    return zmk_behavior_invoke_binding(&combo->behavior, event, true);
}

static inline int release_combo_behavior(int combo_idx, const struct combo_cfg *combo,
                                         int32_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = ZMK_VIRTUAL_KEY_POSITION_COMBO(combo_idx),
        .timestamp = timestamp,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };

    return zmk_behavior_invoke_binding(&combo->behavior, event, false);
}

static void move_pressed_keys_to_active_combo(struct active_combo *active_combo) {

    int combo_length =
        MIN(pressed_keys_count, get_combo_with_id(active_combo->combo_idx)->key_position_len);
    for (int i = 0; i < combo_length; i++) {
        active_combo->key_positions_pressed[i] = pressed_keys[i];
    }
    active_combo->key_positions_pressed_count = combo_length;

    // move any other pressed keys up
    for (int i = 0; i + combo_length < pressed_keys_count; i++) {
        pressed_keys[i] = pressed_keys[i + combo_length];
    }

    pressed_keys_count -= combo_length;
}

static struct active_combo *store_active_combo(int32_t combo_idx) {
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS; i++) {
        if (active_combos[i].combo_idx == UINT16_MAX) {
            active_combos[i].combo_idx = combo_idx;
            active_combo_count++;
            return &active_combos[i];
        }
    }
    LOG_ERR("Unable to store combo; already %d active. Increase "
            "CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS",
            CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS);
    return NULL;
}

static void activate_combo(int combo_idx) {
    struct active_combo *active_combo = store_active_combo(combo_idx);
    if (active_combo == NULL) {
        // unable to store combo
        release_pressed_keys();
        return;
    }
    move_pressed_keys_to_active_combo(active_combo);
    press_combo_behavior(combo_idx, get_combo_with_id(combo_idx),
                         active_combo->key_positions_pressed[0].data.timestamp);
}

static void deactivate_combo(int active_combo_index) {
    active_combo_count--;
    if (active_combo_index != active_combo_count) {
        memcpy(&active_combos[active_combo_index], &active_combos[active_combo_count],
               sizeof(struct active_combo));
    }
    active_combos[active_combo_count] = (struct active_combo){0};
    active_combos[active_combo_count].combo_idx = UINT16_MAX;
}

/* returns true if a key was released. */
static bool release_combo_key(int32_t position, int64_t timestamp) {
    for (int combo_idx = 0; combo_idx < active_combo_count; combo_idx++) {
        struct active_combo *active_combo = &active_combos[combo_idx];

        bool key_released = false;
        bool all_keys_pressed = active_combo->key_positions_pressed_count ==
                                get_combo_with_id(active_combo->combo_idx)->key_position_len;
        bool all_keys_released = true;
        for (int i = 0; i < active_combo->key_positions_pressed_count; i++) {
            if (key_released) {
                active_combo->key_positions_pressed[i - 1] = active_combo->key_positions_pressed[i];
                all_keys_released = false;
            } else if (active_combo->key_positions_pressed[i].data.position != position) {
                all_keys_released = false;
            } else { // position matches
                key_released = true;
            }
        }

        if (key_released) {
            active_combo->key_positions_pressed_count--;
            const struct combo_cfg *c = get_combo_with_id(active_combo->combo_idx);
            if ((c->slow_release && all_keys_released) || (!c->slow_release && all_keys_pressed)) {
                release_combo_behavior(active_combo->combo_idx, c, timestamp);
            }
            if (all_keys_released) {
                deactivate_combo(combo_idx);
            }
            return true;
        }
    }
    return false;
}

static int cleanup() {
    k_work_cancel_delayable(&timeout_task);
    memset(candidates, 0, BYTES_FOR_COMBOS_MASK * sizeof(uint32_t));
    if (fully_pressed_combo != INT16_MAX) {
        activate_combo(fully_pressed_combo);
        fully_pressed_combo = INT16_MAX;
    }
    return release_pressed_keys();
}

static void update_timeout_task() {
    int64_t first_timeout = first_candidate_timeout();
    if (timeout_task_timeout_at == first_timeout) {
        return;
    }
    if (first_timeout == LLONG_MAX) {
        timeout_task_timeout_at = 0;
        k_work_cancel_delayable(&timeout_task);
        return;
    }
    if (k_work_schedule(&timeout_task, K_MSEC(first_timeout - k_uptime_get())) >= 0) {
        timeout_task_timeout_at = first_timeout;
    }
}

static int position_state_down(const zmk_event_t *ev, struct zmk_position_state_changed *data) {
    int num_candidates;
    if (!pressed_keys_count) {
        num_candidates = setup_candidates_for_first_keypress(data->position, data->timestamp);
        if (num_candidates == 0) {
            return ZMK_EV_EVENT_BUBBLE;
        }
    } else {
        filter_timed_out_candidates(data->timestamp);
        num_candidates = filter_candidates(data->position);
    }

    LOG_DBG("combo: capturing position event %d", data->position);
    int ret = capture_pressed_key(data);
    update_timeout_task();

    if (num_candidates) {
        for (int i = 0; i < ZMK_COMBOS_LEN; i++) {
            const struct combo_cfg *candidate_combo = get_combo_with_id(i);
            if (candidate_combo->key_position_len == 0) {
                break;
            }

            if (sys_bitfield_test_bit((mem_addr_t)&candidates, i)) {
                if (candidate_is_completely_pressed(candidate_combo)) {
                    fully_pressed_combo = i;
                    if (num_candidates == 1) {
                        cleanup();
                    }
                }

                return ret;
            }
        }
    } else {
        cleanup();
        return ret;
    }

    return -EINVAL;
}

static int position_state_up(const zmk_event_t *ev, struct zmk_position_state_changed *data) {
    int released_keys = cleanup();
    if (release_combo_key(data->position, data->timestamp)) {
        return ZMK_EV_EVENT_HANDLED;
    }
    if (released_keys > 1) {
        // The second and further key down events are re-raised. To preserve
        // correct order for e.g. hold-taps, reraise the key up event too.
        struct zmk_position_state_changed_event dupe_ev =
            copy_raised_zmk_position_state_changed(data);
        ZMK_EVENT_RAISE(dupe_ev);
        return ZMK_EV_EVENT_CAPTURED;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static void combo_timeout_handler(struct k_work *item) {
    if (timeout_task_timeout_at == 0 || k_uptime_get() < timeout_task_timeout_at) {
        // timer was cancelled or rescheduled.
        return;
    }
    if (filter_timed_out_candidates(timeout_task_timeout_at) == 0) {
        LOG_DBG("CLEANUP!");
        cleanup();
    }

    LOG_DBG("ABOUT TO UPDATE IN TIMEOUT");
    update_timeout_task();
}

static int position_state_changed_listener(const zmk_event_t *ev) {
    struct zmk_position_state_changed *data = as_zmk_position_state_changed(ev);
    if (data == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (data->state) { // keydown
        return position_state_down(ev, data);
    } else { // keyup
        return position_state_up(ev, data);
    }
}

static int keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev->state && !is_mod(ev->usage_page, ev->keycode)) {
        store_last_tapped(ev->timestamp);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

int behavior_combo_listener(const zmk_event_t *eh) {
    if (as_zmk_position_state_changed(eh) != NULL) {
        return position_state_changed_listener(eh);
    } else if (as_zmk_keycode_state_changed(eh) != NULL) {
        return keycode_state_changed_listener(eh);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(combo, behavior_combo_listener);
ZMK_SUBSCRIPTION(combo, zmk_position_state_changed);
ZMK_SUBSCRIPTION(combo, zmk_keycode_state_changed);

static int combo_init(void) {
    for (size_t i = 0; i < CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS; i++) {
        active_combos[i].combo_idx = UINT16_MAX;
    }

    k_work_init_delayable(&timeout_task, combo_timeout_handler);

#if IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    reload_from_static();
#else
    for (int i = 0; i < ARRAY_SIZE(combos); i++) {
        initialize_combo(i);
    }
#endif
    return 0;
}

SYS_INIT(combo_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
