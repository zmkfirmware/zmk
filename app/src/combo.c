/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_combos

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/dlist.h>
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

struct combo_cfg {
    int32_t key_positions[CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO];
    int32_t key_position_len;
    struct zmk_behavior_binding behavior;
    int32_t timeout_ms;
    int32_t require_prior_idle_ms;
    // if slow release is set, the combo releases when the last key is released.
    // otherwise, the combo releases when the first key is released.
    bool slow_release;
    // the virtual key position is a key position outside the range used by the keyboard.
    // it is necessary so hold-taps can uniquely identify a behavior.
    int32_t virtual_key_position;
    int32_t layers_len;
    int8_t layers[];
};

struct active_combo {
    struct combo_cfg *combo;
    // key_positions_pressed is filled with key_positions when the combo is pressed.
    // The keys are removed from this array when they are released.
    // Once this array is empty, the behavior is released.
    const zmk_event_t *key_positions_pressed[CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO];
};

struct combo_candidate {
    struct combo_cfg *combo;
    // the time after which this behavior should be removed from candidates.
    // by keeping track of when the candidate should be cleared there is no
    // possibility of accidental releases.
    int64_t timeout_at;
};

// set of keys pressed
const zmk_event_t *pressed_keys[CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO] = {NULL};
// the set of candidate combos based on the currently pressed_keys
struct combo_candidate candidates[CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY];
// the last candidate that was completely pressed
struct combo_cfg *fully_pressed_combo = NULL;
// a lookup dict that maps a key position to all combos on that position
struct combo_cfg *combo_lookup[ZMK_KEYMAP_LEN][CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY] = {NULL};
// combos that have been activated and still have (some) keys pressed
// this array is always contiguous from 0.
struct active_combo active_combos[CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS] = {NULL};
int active_combo_count = 0;

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
static int initialize_combo(struct combo_cfg *new_combo) {
    for (int i = 0; i < new_combo->key_position_len; i++) {
        int32_t position = new_combo->key_positions[i];
        if (position >= ZMK_KEYMAP_LEN) {
            LOG_ERR("Unable to initialize combo, key position %d does not exist", position);
            return -EINVAL;
        }

        struct combo_cfg *insert_combo = new_combo;
        bool set = false;
        for (int j = 0; j < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY; j++) {
            struct combo_cfg *combo_at_j = combo_lookup[position][j];
            if (combo_at_j == NULL) {
                combo_lookup[position][j] = insert_combo;
                set = true;
                break;
            }
            if (combo_at_j->key_position_len < insert_combo->key_position_len ||
                (combo_at_j->key_position_len == insert_combo->key_position_len &&
                 combo_at_j->virtual_key_position < insert_combo->virtual_key_position)) {
                continue;
            }
            // put insert_combo in this spot, move all other combos up.
            combo_lookup[position][j] = insert_combo;
            insert_combo = combo_at_j;
        }
        if (!set) {
            LOG_ERR("Too many combos for key position %d, CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY %d.",
                    position, CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY);
            return -ENOMEM;
        }
    }
    return 0;
}

static bool combo_active_on_layer(struct combo_cfg *combo, uint8_t layer) {
    if (combo->layers[0] == -1) {
        // -1 in the first layer position is global layer scope
        return true;
    }
    for (int j = 0; j < combo->layers_len; j++) {
        if (combo->layers[j] == layer) {
            return true;
        }
    }
    return false;
}

static bool is_quick_tap(struct combo_cfg *combo, int64_t timestamp) {
    return (last_tapped_timestamp + combo->require_prior_idle_ms) > timestamp;
}

static int setup_candidates_for_first_keypress(int32_t position, int64_t timestamp) {
    int number_of_combo_candidates = 0;
    uint8_t highest_active_layer = zmk_keymap_highest_layer_active();
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY; i++) {
        struct combo_cfg *combo = combo_lookup[position][i];
        if (combo == NULL) {
            return number_of_combo_candidates;
        }
        if (combo_active_on_layer(combo, highest_active_layer) && !is_quick_tap(combo, timestamp)) {
            candidates[number_of_combo_candidates].combo = combo;
            candidates[number_of_combo_candidates].timeout_at = timestamp + combo->timeout_ms;
            number_of_combo_candidates++;
        }
        // LOG_DBG("combo timeout %d %d %d", position, i, candidates[i].timeout_at);
    }
    return number_of_combo_candidates;
}

static int filter_candidates(int32_t position) {
    // this code iterates over candidates and the lookup together to filter in O(n)
    // assuming they are both sorted on key_position_len, virtal_key_position
    int matches = 0, lookup_idx = 0, candidate_idx = 0;
    while (lookup_idx < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY &&
           candidate_idx < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY) {
        struct combo_cfg *candidate = candidates[candidate_idx].combo;
        struct combo_cfg *lookup = combo_lookup[position][lookup_idx];
        if (candidate == NULL || lookup == NULL) {
            break;
        }
        if (candidate->virtual_key_position == lookup->virtual_key_position) {
            candidates[matches] = candidates[candidate_idx];
            matches++;
            candidate_idx++;
            lookup_idx++;
        } else if (candidate->key_position_len > lookup->key_position_len) {
            lookup_idx++;
        } else if (candidate->key_position_len < lookup->key_position_len) {
            candidate_idx++;
        } else if (candidate->virtual_key_position > lookup->virtual_key_position) {
            lookup_idx++;
        } else if (candidate->virtual_key_position < lookup->virtual_key_position) {
            candidate_idx++;
        }
    }
    // clear unmatched candidates
    for (int i = matches; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY; i++) {
        candidates[i].combo = NULL;
    }
    // LOG_DBG("combo matches after filter %d", matches);
    return matches;
}

static int64_t first_candidate_timeout() {
    int64_t first_timeout = LONG_MAX;
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY; i++) {
        if (candidates[i].combo == NULL) {
            break;
        }
        if (candidates[i].timeout_at < first_timeout) {
            first_timeout = candidates[i].timeout_at;
        }
    }
    return first_timeout;
}

static inline bool candidate_is_completely_pressed(struct combo_cfg *candidate) {
    // this code assumes set(pressed_keys) <= set(candidate->key_positions)
    // this invariant is enforced by filter_candidates
    // since events may have been reraised after clearing one or more slots at
    // the start of pressed_keys (see: release_pressed_keys), we have to check
    // that each key needed to trigger the combo was pressed, not just the last.
    for (int i = 0; i < candidate->key_position_len; i++) {
        if (pressed_keys[i] == NULL) {
            return false;
        }
    }
    return true;
}

static int cleanup();

static int filter_timed_out_candidates(int64_t timestamp) {
    int remaining_candidates = 0;
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY; i++) {
        struct combo_candidate *candidate = &candidates[i];
        if (candidate->combo == NULL) {
            break;
        }
        if (candidate->timeout_at > timestamp) {
            bool need_to_bubble_up = remaining_candidates != i;
            if (need_to_bubble_up) {
                // bubble up => reorder candidates so they're contiguous
                candidates[remaining_candidates].combo = candidate->combo;
                candidates[remaining_candidates].timeout_at = candidate->timeout_at;
                // clear the previous location
                candidates[i].combo = NULL;
                candidates[i].timeout_at = 0;
            }

            remaining_candidates++;
        } else {
            candidate->combo = NULL;
        }
    }

    LOG_DBG(
        "after filtering out timed out combo candidates: remaining_candidates=%d timestamp=%lld",
        remaining_candidates, timestamp);

    return remaining_candidates;
}

static int clear_candidates() {
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY; i++) {
        if (candidates[i].combo == NULL) {
            return i;
        }
        candidates[i].combo = NULL;
    }
    return CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY;
}

static int capture_pressed_key(const zmk_event_t *ev) {
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO; i++) {
        if (pressed_keys[i] != NULL) {
            continue;
        }
        pressed_keys[i] = ev;
        return ZMK_EV_EVENT_CAPTURED;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

const struct zmk_listener zmk_listener_combo;

static int release_pressed_keys() {
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO; i++) {
        const zmk_event_t *captured_event = pressed_keys[i];
        if (pressed_keys[i] == NULL) {
            return i;
        }
        pressed_keys[i] = NULL;
        if (i == 0) {
            LOG_DBG("combo: releasing position event %d",
                    as_zmk_position_state_changed(captured_event)->position);
            ZMK_EVENT_RELEASE(captured_event)
        } else {
            // reprocess events (see tests/combo/fully-overlapping-combos-3 for why this is needed)
            LOG_DBG("combo: reraising position event %d",
                    as_zmk_position_state_changed(captured_event)->position);
            ZMK_EVENT_RAISE(captured_event);
        }
    }
    return CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO;
}

static inline int press_combo_behavior(struct combo_cfg *combo, int32_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = combo->virtual_key_position,
        .timestamp = timestamp,
    };

    last_combo_timestamp = timestamp;

    return behavior_keymap_binding_pressed(&combo->behavior, event);
}

static inline int release_combo_behavior(struct combo_cfg *combo, int32_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = combo->virtual_key_position,
        .timestamp = timestamp,
    };

    return behavior_keymap_binding_released(&combo->behavior, event);
}

static void move_pressed_keys_to_active_combo(struct active_combo *active_combo) {
    int combo_length = active_combo->combo->key_position_len;
    for (int i = 0; i < combo_length; i++) {
        active_combo->key_positions_pressed[i] = pressed_keys[i];
        pressed_keys[i] = NULL;
    }
    // move any other pressed keys up
    for (int i = 0; i + combo_length < CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO; i++) {
        if (pressed_keys[i + combo_length] == NULL) {
            return;
        }
        pressed_keys[i] = pressed_keys[i + combo_length];
        pressed_keys[i + combo_length] = NULL;
    }
}

static struct active_combo *store_active_combo(struct combo_cfg *combo) {
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS; i++) {
        if (active_combos[i].combo == NULL) {
            active_combos[i].combo = combo;
            active_combo_count++;
            return &active_combos[i];
        }
    }
    LOG_ERR("Unable to store combo; already %d active. Increase "
            "CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS",
            CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS);
    return NULL;
}

static void activate_combo(struct combo_cfg *combo) {
    struct active_combo *active_combo = store_active_combo(combo);
    if (active_combo == NULL) {
        // unable to store combo
        release_pressed_keys();
        return;
    }
    move_pressed_keys_to_active_combo(active_combo);
    press_combo_behavior(
        combo, as_zmk_position_state_changed(active_combo->key_positions_pressed[0])->timestamp);
}

static void deactivate_combo(int active_combo_index) {
    active_combo_count--;
    if (active_combo_index != active_combo_count) {
        memcpy(&active_combos[active_combo_index], &active_combos[active_combo_count],
               sizeof(struct active_combo));
    }
    active_combos[active_combo_count].combo = NULL;
    active_combos[active_combo_count] = (struct active_combo){0};
}

/* returns true if a key was released. */
static bool release_combo_key(int32_t position, int64_t timestamp) {
    for (int combo_idx = 0; combo_idx < active_combo_count; combo_idx++) {
        struct active_combo *active_combo = &active_combos[combo_idx];

        bool key_released = false;
        bool all_keys_pressed = true;
        bool all_keys_released = true;
        for (int i = 0; i < active_combo->combo->key_position_len; i++) {
            if (active_combo->key_positions_pressed[i] == NULL) {
                all_keys_pressed = false;
            } else if (as_zmk_position_state_changed(active_combo->key_positions_pressed[i])
                           ->position != position) {
                all_keys_released = false;
            } else { // not null and position matches
                ZMK_EVENT_FREE(active_combo->key_positions_pressed[i]);
                active_combo->key_positions_pressed[i] = NULL;
                key_released = true;
            }
        }

        if (key_released) {
            if ((active_combo->combo->slow_release && all_keys_released) ||
                (!active_combo->combo->slow_release && all_keys_pressed)) {
                release_combo_behavior(active_combo->combo, timestamp);
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
    clear_candidates();
    if (fully_pressed_combo != NULL) {
        activate_combo(fully_pressed_combo);
        fully_pressed_combo = NULL;
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
    if (candidates[0].combo == NULL) {
        num_candidates = setup_candidates_for_first_keypress(data->position, data->timestamp);
        if (num_candidates == 0) {
            return ZMK_EV_EVENT_BUBBLE;
        }
    } else {
        filter_timed_out_candidates(data->timestamp);
        num_candidates = filter_candidates(data->position);
    }
    update_timeout_task();

    struct combo_cfg *candidate_combo = candidates[0].combo;
    LOG_DBG("combo: capturing position event %d", data->position);
    int ret = capture_pressed_key(ev);
    switch (num_candidates) {
    case 0:
        cleanup();
        return ret;
    case 1:
        if (candidate_is_completely_pressed(candidate_combo)) {
            fully_pressed_combo = candidate_combo;
            cleanup();
        }
        return ret;
    default:
        if (candidate_is_completely_pressed(candidate_combo)) {
            fully_pressed_combo = candidate_combo;
        }
        return ret;
    }
}

static int position_state_up(const zmk_event_t *ev, struct zmk_position_state_changed *data) {
    int released_keys = cleanup();
    if (release_combo_key(data->position, data->timestamp)) {
        return ZMK_EV_EVENT_HANDLED;
    }
    if (released_keys > 1) {
        // The second and further key down events are re-raised. To preserve
        // correct order for e.g. hold-taps, reraise the key up event too.
        ZMK_EVENT_RAISE(ev);
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
        cleanup();
    }
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

#define COMBO_INST(n)                                                                              \
    static struct combo_cfg combo_config_##n = {                                                   \
        .timeout_ms = DT_PROP(n, timeout_ms),                                                      \
        .require_prior_idle_ms = DT_PROP(n, require_prior_idle_ms),                                \
        .key_positions = DT_PROP(n, key_positions),                                                \
        .key_position_len = DT_PROP_LEN(n, key_positions),                                         \
        .behavior = ZMK_KEYMAP_EXTRACT_BINDING(0, n),                                              \
        .virtual_key_position = ZMK_VIRTUAL_KEY_POSITION_COMBO(__COUNTER__),                       \
        .slow_release = DT_PROP(n, slow_release),                                                  \
        .layers = DT_PROP(n, layers),                                                              \
        .layers_len = DT_PROP_LEN(n, layers),                                                      \
    };

#define INITIALIZE_COMBO(n) initialize_combo(&combo_config_##n);

DT_INST_FOREACH_CHILD(0, COMBO_INST)

static int combo_init() {
    k_work_init_delayable(&timeout_task, combo_timeout_handler);
    DT_INST_FOREACH_CHILD(0, INITIALIZE_COMBO);
    return 0;
}

SYS_INIT(combo_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
