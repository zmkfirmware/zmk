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
#include <zmk/events/action_behavior_triggered.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>
#include <zmk/virtual_key_position.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct combo_cfg {
    // Mandatory DT props
    int32_t *triggered_by; // DT 'triggers' array
    uint8_t triggered_by_len;

    struct zmk_behavior_binding behavior; // trigger on successful combo
    int32_t timeout_ms;

    // Additional data

    // the virtual key position is a key position outside the range used by the keyboard.
    // it is necessary so hold-taps can uniquely identify a behavior.
    int32_t virtual_key_position;

    // Optional DT props
    int32_t require_prior_idle_ms;
    bool slow_release; // release combo when (T:all, F:one) trigger released
};

struct active_combo {
    struct combo_cfg *combo;
// Marks which of the triggers in the combo are currently active
// used for slow_release and to prevent combo re-entry before all triggers released
// Bit flags corresponding to locations in the 'triggered_by' array
// (1<<x)-1 is used, hence CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO has max val of 31
#if CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO < 31
#if CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO < 16
#if CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO < 8
    uint8_t active_triggers; // I suspect that this suffices for a vast majority of cases
#else
    uint16_t active_triggers;
#endif
#else
    uint32_t active_triggers;
#endif
#else
    uint64_t active_triggers;
#endif
};

/*
 * Struct used to store triggers.
 * Stored information is used to trigger the fallback behavior event when a combo fails
 * to activate due to timeout/not all triggers being active.
 */
struct trigger {
    uint32_t trigger_id;
    char *fallback_behavior_dev;
    uint32_t fallback_param;
    struct zmk_behavior_binding_event event;
};

// list of stored triggers
struct trigger stored_triggers[CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO] = {};
size_t stored_triggers_count = 0;
// the set of candidate combos based on the currently stored triggers
// sorted by combo length followed by virtual key position
struct combo_cfg *candidates[CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER] = {NULL};
// The timestamp that a combo actuates at, taken from the first stored trigger
int64_t candidate_timestamp = 0;
// the most recent candidate to have all of its triggers stored, using the candidate order
static struct combo_cfg *fully_pressed_combo = NULL;
// a lookup dict that maps a trigger to all combos it could trigger
struct combo_cfg *combo_lookup[CONFIG_ZMK_COMBO_MAX_TRIGGER_NUM]
                              [CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER] = {NULL};
// combos that have been activated and still have (some) keys pressed
// this array is always contiguous from 0.
struct active_combo active_combos[CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS] = {NULL};
int active_combo_count = 0;

// used to prevent combo from entering a capture->trigger loop when activating a behavior
bool combo_is_being_activated = false;
// This event exists to allow hold-tap interrupts etc. to work
// It gets raised when combo invokes a behavior, and resolves when it reaches combo again
// Start with negative timestamp to avoid issues with key_position 0 being pressed at time 0
struct zmk_position_state_changed combo_position_changed_notification = {
    .timestamp = -1,
};

// used for 'timeout-ms'
struct k_work_delayable timeout_task;
int64_t timeout_task_timeout_at;

//---
/*
 * Used for 'require-prior-idle-ms'
 */
// this keeps track of the last non-combo, non-mod key tap
int64_t last_tapped_timestamp = INT32_MIN;
// this keeps track of the last time a combo was pressed
int64_t last_combo_timestamp = INT32_MIN;

static void store_last_tapped(int64_t timestamp) {
    if (timestamp > last_combo_timestamp) {
        last_tapped_timestamp = timestamp;
    }
}
//---

// Store the combo key pointer in the combo lookup array, one pointer for each trigger
// The combos are sorted shortest-first, then by virtual-key-position.
static int initialize_combo(struct combo_cfg *new_combo) {
    for (int i = 0; i < new_combo->triggered_by_len; i++) {
        int32_t trigger_id = new_combo->triggered_by[i];
        if (trigger_id >= CONFIG_ZMK_COMBO_MAX_TRIGGER_NUM) {
            LOG_ERR("Unable to initialize combo, trigger %d does not exist", trigger_id);
            return -EINVAL;
        }

        // insort the combo
        struct combo_cfg *insert_combo = new_combo;
        bool set = false;
        for (int j = 0; j < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER; j++) {
            struct combo_cfg *combo_at_j = combo_lookup[trigger_id][j];
            if (combo_at_j == NULL) {
                combo_lookup[trigger_id][j] = insert_combo;
                set = true;
                break;
            }
            if (combo_at_j->triggered_by_len < insert_combo->triggered_by_len ||
                (combo_at_j->triggered_by_len == insert_combo->triggered_by_len &&
                 combo_at_j->virtual_key_position < insert_combo->virtual_key_position)) {
                continue;
            }
            // put insert_combo in this spot, move all other combos up.
            combo_lookup[trigger_id][j] = insert_combo;
            insert_combo = combo_at_j;
        }

        if (!set) {
            LOG_ERR("Too many combos for trigger %d, CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER %d.",
                    trigger_id, CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER);
            return -ENOMEM;
        }
    }
    return 0;
}

// Used for 'require-prior-idle-ms'
static bool is_quick_tap(struct combo_cfg *combo, int64_t timestamp) {
    return (last_tapped_timestamp + combo->require_prior_idle_ms) > timestamp;
}

// candidates will be sorted as combo_lookup is sorted
static int identify_initial_candidates(int32_t trigger_id, int64_t timestamp) {
    int number_of_combo_candidates = 0;
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER; i++) {
        struct combo_cfg *combo = combo_lookup[trigger_id][i];
        if (combo == NULL) {
            return number_of_combo_candidates;
        }
        if (!is_quick_tap(combo, timestamp)) {
            candidates[number_of_combo_candidates] = combo;
            number_of_combo_candidates++;
        }
    }
    return number_of_combo_candidates;
}

static int filter_candidates(int32_t trigger_id) {
    // this code iterates over candidates and the lookup together to filter in O(n)
    // assuming they are both sorted on triggered_by_len, virtual_key_position
    int matches = 0, lookup_idx = 0, candidate_idx = 0;
    while (lookup_idx < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER &&
           candidate_idx < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER) {
        struct combo_cfg *candidate = candidates[candidate_idx];
        struct combo_cfg *lookup = combo_lookup[trigger_id][lookup_idx];
        if (candidate == NULL || lookup == NULL) {
            break;
        }
        if (candidate->virtual_key_position == lookup->virtual_key_position) {
            candidates[matches] = candidates[candidate_idx];
            matches++;
            candidate_idx++;
            lookup_idx++;
        } else if (candidate->triggered_by_len > lookup->triggered_by_len) {
            lookup_idx++;
        } else if (candidate->triggered_by_len < lookup->triggered_by_len) {
            candidate_idx++;
        } else if (candidate->virtual_key_position > lookup->virtual_key_position) {
            lookup_idx++;
        } else if (candidate->virtual_key_position < lookup->virtual_key_position) {
            candidate_idx++;
        }
    }
    // clear unmatched candidates
    for (int i = matches; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER; i++) {
        candidates[i] = NULL;
    }
    // LOG_DBG("combo matches after filter %d", matches);
    return matches;
}

static int64_t first_candidate_timeout() {
    int64_t first_timeout = LONG_MAX;
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER; i++) {
        if (candidates[i] == NULL) {
            break;
        }
        int64_t candidate_timeout = candidate_timestamp + candidates[i]->timeout_ms;
        if (candidate_timeout < first_timeout) {
            first_timeout = candidate_timeout;
        }
    }
    return first_timeout;
}

static inline bool candidate_is_completely_pressed(struct combo_cfg *candidate) {
    /**
     * This code assumes that set(active_triggers) <= set(candidate->triggered_by).
     * This invariant is enforced by filter_candidates
     * Note that a completely pressed candidate is removed from candidates,
     * so the case where active_triggers has more elements than triggered_by cannot exist
     * If this changes in the future, change the equality to a <=
     */
    return candidate->triggered_by_len == stored_triggers_count;
}

static int filter_timed_out_candidates(int64_t timestamp) {
    int remaining_candidates = 0;
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER; i++) {
        struct combo_cfg *candidate = candidates[i];
        if (candidate == NULL) {
            break;
        }
        if (candidate_timestamp + candidate->timeout_ms > timestamp) {
            bool need_to_bubble_up = remaining_candidates != i;
            if (need_to_bubble_up) {
                // bubble up => reorder candidates so they're contiguous
                candidates[remaining_candidates] = candidate;
                // clear the previous location
                candidates[i] = NULL;
            }

            remaining_candidates++;
        } else {
            candidates[i] = NULL;
        }
    }

    LOG_DBG(
        "after filtering out timed out combo candidates: remaining_candidates=%d timestamp=%lld",
        remaining_candidates, timestamp);

    return remaining_candidates;
}

static inline int clear_candidates() {
    for (int i = 0; i < CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER; i++) {
        if (candidates[i] == NULL) {
            return i;
        }
        candidates[i] = NULL;
    }
    return CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER;
}

static inline int raise_combo_state_changed(bool state, int position, int timestamp) {
    struct zmk_position_state_changed notification_event = {
        .state = state,
        .position = position,
        .timestamp = timestamp,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };
    LOG_DBG("combo: raising position state %d", position);
    combo_position_changed_notification = notification_event;
    return raise_zmk_position_state_changed(notification_event);
}

// needs to trigger a pos event
static inline int press_fallback_behavior(struct trigger *trigger) {
    struct zmk_behavior_binding binding = {.behavior_dev = trigger->fallback_behavior_dev,
                                           .param1 = trigger->fallback_param};

    raise_combo_state_changed(true, trigger->event.position, trigger->event.timestamp);
    return zmk_behavior_invoke_binding(&binding, trigger->event, true);
}

// needs to trigger a pos event
static inline int release_fallback_behavior(struct trigger *trigger) {
    struct zmk_behavior_binding binding = {.behavior_dev = trigger->fallback_behavior_dev,
                                           .param1 = trigger->fallback_param};
    raise_combo_state_changed(false, trigger->event.position, trigger->event.timestamp);
    return zmk_behavior_invoke_binding(&binding, trigger->event, false);
}

// needs to trigger a pos event on failure
static inline int capture_trigger(struct trigger *trigger) {
    if (stored_triggers_count == CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER) {
        return press_fallback_behavior(trigger);
    }
    stored_triggers[stored_triggers_count++] = *trigger;
    return ZMK_BEHAVIOR_OPAQUE;
}

// Declare this early, to allow reuse in the below function
static int trigger_state_down(int trigger_id, int64_t timestamp, struct trigger *trigger);

static int release_stored_triggers(int stored_triggers_offset) {
    // Offset allows the reprocessing of superfluous triggers on combo activation
    uint32_t count = stored_triggers_offset + stored_triggers_count;
    stored_triggers_count = 0;
    for (int i = stored_triggers_offset; i < count; i++) {
        struct trigger trigger = stored_triggers[i];
        if (i == 0) {
            LOG_DBG("combo: activating fallback behavior from trigger %d", trigger.trigger_id);
            press_fallback_behavior(&trigger);
        } else {
            // reprocess trigger
            //(see tests/combo/fully-overlapping-combos-3 for why this is needed)
            LOG_DBG("combo: reprocessing trigger %d", trigger.trigger_id);
            trigger_state_down(trigger.trigger_id, trigger.event.timestamp, &stored_triggers[i]);
        }
    }
    return count - stored_triggers_offset;
}

// needs to trigger a pos event
static inline int press_combo_behavior(struct combo_cfg *combo, int32_t timestamp) {
    last_combo_timestamp = timestamp;

    struct zmk_behavior_binding_event event = {
        .position = combo->virtual_key_position,
        .timestamp = timestamp,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };
    raise_combo_state_changed(true, combo->virtual_key_position, k_uptime_get());
    return zmk_behavior_invoke_binding(&combo->behavior, event, true);
}

// needs to trigger a pos event
static inline int release_combo_behavior(struct combo_cfg *combo, int32_t timestamp) {

    struct zmk_behavior_binding_event event = {
        .position = combo->virtual_key_position,
        .timestamp = timestamp,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };

    raise_combo_state_changed(false, combo->virtual_key_position, k_uptime_get());
    return zmk_behavior_invoke_binding(&combo->behavior, event, false);
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

static int activate_combo(struct combo_cfg *combo) {
    // returns the number of triggers used to activate the combo
    struct active_combo *active_combo = store_active_combo(combo);
    if (active_combo == NULL) {
        return 0;
    }
    int combo_count = active_combo->combo->triggered_by_len;
    active_combo->active_triggers = (1 << combo_count) - 1;
    stored_triggers_count -= combo_count;
    combo_is_being_activated = true;
    press_combo_behavior(combo, candidate_timestamp);
    combo_is_being_activated = false;
    return combo_count;
}

// Removes combo from active_combos array
// keeps active_combos contiguous from 0
static void deactivate_combo(int active_combo_index) {
    active_combo_count--;
    if (active_combo_index != active_combo_count) {
        memcpy(&active_combos[active_combo_index], &active_combos[active_combo_count],
               sizeof(struct active_combo));
    }
    active_combos[active_combo_count].combo = NULL;
    // No need to overwrite active_triggers, it will get overwritten when a new combo is activated
}

/* returns true if a key was released.
releases combo, specific approach depends on slow_release
Each trigger can only activate one combo at once TODO
deactivates combo if all keys were released
*/
static bool release_combo_key(int32_t trigger_id, int64_t timestamp) {
    for (int combo_idx = 0; combo_idx < active_combo_count; combo_idx++) {
        struct active_combo *active_combo = &active_combos[combo_idx];

        bool trigger_was_released = false;
        bool first_released_trigger =
            active_combo->active_triggers == (1 << active_combo->combo->triggered_by_len) - 1;

        for (int i = 0; i < active_combo->combo->triggered_by_len; i++) {
            if (active_combo->combo->triggered_by[i] == trigger_id) {
                active_combo->active_triggers &= ~(1 << i);
                trigger_was_released = true;
                break;
            }
        }
        bool last_released_trigger = active_combo->active_triggers == 0;

        if (trigger_was_released) {
            if ((active_combo->combo->slow_release && last_released_trigger) ||
                (!active_combo->combo->slow_release && first_released_trigger)) {
                release_combo_behavior(active_combo->combo, timestamp);
            }
            if (last_released_trigger) {
                // deactivate only when all triggers are released
                deactivate_combo(combo_idx);
            }
            return true;
        }
    }
    return false;
}

static int combo_cleanup() {
    if (combo_is_being_activated) {
        return 0;
    }
    k_work_cancel_delayable(&timeout_task);
    int offset = 0;
    if (fully_pressed_combo != NULL) {
        offset = activate_combo(fully_pressed_combo);
        fully_pressed_combo = NULL;
    }
    clear_candidates();
    return release_stored_triggers(offset);
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

static void combo_timeout_handler(struct k_work *item) {
    if (timeout_task_timeout_at == 0 || k_uptime_get() < timeout_task_timeout_at) {
        // timer was cancelled or rescheduled.
        return;
    }
    // If there are no remaining candidates
    if (filter_timed_out_candidates(timeout_task_timeout_at) == 0) {
        combo_cleanup();
    }
    update_timeout_task();
}

// checks if a behavior trigger has already been stored
static bool is_valid_trigger(struct trigger *trigger) {
    for (int i = 0; i < stored_triggers_count; i++) {
        if (trigger->trigger_id == stored_triggers[i].trigger_id) {
            return false;
        }
    }
    return true;
}

static int trigger_state_down(int trigger_id, int64_t timestamp, struct trigger *trigger) {
    int num_candidates;
    if (candidates[0] == NULL) {
        num_candidates = identify_initial_candidates(trigger_id, timestamp);
        if (num_candidates == 0) {
            return press_fallback_behavior(trigger);
        }
        candidate_timestamp = timestamp;
    } else {
        if (!is_valid_trigger(trigger)) {
            LOG_DBG("combo: invalid trigger %d", trigger->trigger_id);
            combo_cleanup();
            return press_fallback_behavior(trigger);
        }
        filter_timed_out_candidates(timestamp);
        num_candidates = filter_candidates(trigger_id); // does nothing to timing
    }

    struct combo_cfg *candidate_combo = candidates[0];
    LOG_DBG("combo: capturing trigger %d", trigger_id);

    int ret = capture_trigger(trigger);
    update_timeout_task();
    switch (num_candidates) {
    case 0:
        combo_cleanup();
        return ret;
    case 1: // early accept when only one combo remains
        if (candidate_is_completely_pressed(candidate_combo)) {
            fully_pressed_combo = candidate_combo;
            combo_cleanup();
        }
        return ret;
    default:
        if (candidate_is_completely_pressed(candidate_combo)) {
            fully_pressed_combo = candidate_combo;
        }
        return ret;
    }
}

static int trigger_state_up(int trigger_id, int64_t timestamp, struct trigger *trigger) {
    combo_cleanup();
    if (!release_combo_key(trigger_id, timestamp)) {
        release_fallback_behavior(trigger);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

int zmk_combo_trigger_behavior_invoked(int trigger_id, char *fallback_behavior_dev,
                                       uint32_t fallback_param,
                                       struct zmk_behavior_binding_event event, bool state) {
    struct trigger trigger = {.trigger_id = trigger_id,
                              .fallback_behavior_dev = fallback_behavior_dev,
                              .fallback_param = fallback_param,
                              .event = event};
    if (state) { // keydown
        return trigger_state_down(trigger_id, event.timestamp, &trigger);
    } else { // keyup
        return trigger_state_up(trigger_id, event.timestamp, &trigger);
    }
}

static int action_behavior_triggered_listener(const zmk_event_t *ev) {
    struct zmk_action_behavior_triggered *data = as_zmk_action_behavior_triggered(ev);
    if (data == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    while (combo_cleanup()) {
        continue;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev->state && !is_mod(ev->usage_page, ev->keycode)) {
        store_last_tapped(ev->timestamp);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

int behavior_combo_listener(const zmk_event_t *eh) {
    if (as_zmk_action_behavior_triggered(eh) != NULL) {
        return action_behavior_triggered_listener(eh);
    } else if (as_zmk_keycode_state_changed(eh) != NULL) {
        return keycode_state_changed_listener(eh);
    } else if (as_zmk_position_state_changed(eh) != NULL) {
        struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
        if (ev->position == combo_position_changed_notification.position &&
            ev->timestamp == combo_position_changed_notification.timestamp) {
            LOG_DBG("combo: catching position state %d", ev->position);
            return ZMK_EV_EVENT_HANDLED;
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(combo, behavior_combo_listener);
ZMK_SUBSCRIPTION(combo, zmk_position_state_changed);
ZMK_SUBSCRIPTION(combo, zmk_action_behavior_triggered);
ZMK_SUBSCRIPTION(combo, zmk_keycode_state_changed);

#define COMBO_GET_TRIGGER(idx, n) DT_PROP_BY_IDX(n, triggers, idx)

#define COMBO_TRIGGER_ASSERT_VALID(node_id, prop, idx)                                             \
    BUILD_ASSERT((DT_PROP_BY_IDX(node_id, prop, idx) < CONFIG_ZMK_COMBO_MAX_TRIGGER_NUM) &&        \
                     (DT_PROP_BY_IDX(node_id, prop, idx) >= 0),                                    \
                 "Combo has an invalid trigger. ");

#define COMBO_INST(n)                                                                              \
    BUILD_ASSERT(COND_CODE_1(DT_NODE_HAS_PROP(n, triggers), (DT_NODE_HAS_PROP(n, triggers)),       \
                             (DT_NODE_HAS_PROP(n, key_positions))),                                \
                 "Combo is missing the 'triggers' property. ");                                    \
    DT_FOREACH_PROP_ELEM(n, triggers, COMBO_TRIGGER_ASSERT_VALID)                                  \
    BUILD_ASSERT(DT_PROP_LEN(n, triggers) <= CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO,              \
                 "Combo has too many triggers, adjust "                                            \
                 "CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO appropriately.");                        \
    static int32_t combo_config_##n##_triggers[DT_PROP_LEN(n, triggers)] = {                       \
        LISTIFY(DT_PROP_LEN(n, triggers), COMBO_GET_TRIGGER, (, ), n)};                            \
    static struct combo_cfg combo_config_##n = {                                                   \
        .timeout_ms = DT_PROP(n, timeout_ms),                                                      \
        .require_prior_idle_ms = DT_PROP(n, require_prior_idle_ms),                                \
        .triggered_by = combo_config_##n##_triggers,                                               \
        .triggered_by_len = DT_PROP_LEN(n, triggers),                                              \
        .behavior = ZMK_KEYMAP_EXTRACT_BINDING(0, n),                                              \
        .virtual_key_position = ZMK_VIRTUAL_KEY_POSITION_COMBO(__COUNTER__),                       \
        .slow_release = DT_PROP(n, slow_release),                                                  \
    };

#define INITIALIZE_COMBO(n) initialize_combo(&combo_config_##n);

DT_INST_FOREACH_CHILD(0, COMBO_INST)

static int combo_init(void) {
    k_work_init_delayable(&timeout_task, combo_timeout_handler);
    DT_INST_FOREACH_CHILD(0, INITIALIZE_COMBO);
    return 0;
}

SYS_INIT(combo_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
