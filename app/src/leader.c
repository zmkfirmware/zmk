/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_leader_sequences

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/kernel.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>
#include <zmk/leader.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

bool leader_status;
int32_t press_count;
int32_t release_count;
int32_t timeout_ms;
int32_t active_leader_position;
int8_t layer;
bool first_release;
struct k_work_delayable release_timer;
int64_t release_at;
bool timer_started;
bool timer_cancelled;
bool timerless;

struct leader_seq_cfg {
    int32_t key_positions[CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE];
    int32_t key_position_len;

    bool immediate_trigger;
    bool is_pressed;
    // the virtual key position is a key position outside the range used by the keyboard.
    // it is necessary so hold-taps can uniquely identify a behavior.
    int32_t virtual_key_position;
    struct zmk_behavior_binding behavior;
    int32_t layers_len;
    int8_t layers[];
};

// leader_pressed_keys is filled with an event when a key is pressed.
// The keys are removed from this array when they are released.
// Once this array is empty, the behavior is released.
const struct zmk_position_state_changed
    *leader_pressed_keys[CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE] = {NULL};

uint32_t current_sequence[CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE] = {-1};
// the set of candidate leader based on the currently leader_pressed_keys
int num_candidates;
struct leader_seq_cfg *sequence_candidates[CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY];
int num_comp_candidates;
struct leader_seq_cfg *completed_sequence_candidates[CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY];
// a lookup dict that maps a key position to all sequences on that position
struct leader_seq_cfg *sequence_lookup[ZMK_KEYMAP_LEN][CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY] = {
    NULL};

// Store the leader key pointer in the leader array, one pointer for each key position
// The leader are sorted shortest-first, then by virtual-key-position.
static int intitialiaze_leader_sequences(struct leader_seq_cfg *seq) {
    for (int i = 0; i < seq->key_position_len; i++) {
        int32_t position = seq->key_positions[i];
        if (position >= ZMK_KEYMAP_LEN) {
            LOG_ERR("Unable to initialize leader, key position %d does not exist", position);
            return -EINVAL;
        }

        struct leader_seq_cfg *new_seq = seq;
        bool set = false;
        for (int j = 0; j < CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY; j++) {
            struct leader_seq_cfg *sequence_at_j = sequence_lookup[position][j];
            if (sequence_at_j == NULL) {
                sequence_lookup[position][j] = new_seq;
                set = true;
                break;
            }
            if (sequence_at_j->key_position_len < new_seq->key_position_len ||
                (sequence_at_j->key_position_len == new_seq->key_position_len &&
                 sequence_at_j->virtual_key_position < new_seq->virtual_key_position)) {
                continue;
            }
            // put new_seq in this spot, move all other leader up.
            sequence_lookup[position][j] = new_seq;
            new_seq = sequence_at_j;
        }
        if (!set) {
            LOG_ERR(
                "Too many leader for key position %d, CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY %d.",
                position, CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY);
            return -ENOMEM;
        }
    }
    return 0;
}

static bool sequence_active_on_layer(struct leader_seq_cfg *sequence) {
    if (sequence->layers[0] == -1) {
        // -1 in the first layer position is global layer scope
        return true;
    }
    for (int j = 0; j < sequence->layers_len; j++) {
        if (sequence->layers[j] == layer) {
            return true;
        }
    }
    return false;
}

static bool has_current_sequence(struct leader_seq_cfg *sequence, int count) {
    for (int i = 0; i < count; i++) {
        if (sequence->key_positions[i] != current_sequence[i]) {
            return false;
        }
    }
    return true;
}

static bool is_in_current_sequence(int32_t position) {
    for (int i = 0; i < CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE; i++) {
        if (position == current_sequence[i]) {
            return true;
        }
    }
    return false;
}

static bool is_duplicate(struct leader_seq_cfg *seq) {
    for (int i = 0; i < CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE; i++) {
        if (sequence_candidates[i] == seq) {
            return true;
        }
    }
    return false;
}

static bool release_key_in_sequence(int32_t position) {
    for (int i = 0; i < release_count; i++) {
        if (leader_pressed_keys[i] && position == leader_pressed_keys[i]->position) {
            leader_pressed_keys[i] = NULL;
            return true;
        }
    }
    return false;
}

static bool all_keys_released() {
    for (int i = 0; i < press_count; i++) {
        if (NULL != leader_pressed_keys[i]) {
            return false;
        }
    }
    return true;
}

static void clear_candidates() {
    for (int i = 0; i < CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY; i++) {
        sequence_candidates[i] = NULL;
        completed_sequence_candidates[i] = NULL;
    }
}

static void leader_find_candidates(int32_t position, int count) {
    clear_candidates();
    num_candidates = 0;
    num_comp_candidates = 0;
    for (int i = 0; i < CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY; i++) {
        struct leader_seq_cfg *sequence = sequence_lookup[position][i];
        if (sequence == NULL) {
            continue;
        }
        if (sequence_active_on_layer(sequence) && sequence->key_positions[count] == position &&
            has_current_sequence(sequence, count) && !is_duplicate(sequence)) {
            sequence_candidates[num_candidates] = sequence;
            num_candidates++;
            if (sequence->key_position_len == count + 1) {
                completed_sequence_candidates[num_comp_candidates] = sequence;
                num_comp_candidates++;
            }
        }
    }
}

const struct zmk_listener zmk_listener_leader;

static inline int press_leader_behavior(struct leader_seq_cfg *sequence, int32_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = sequence->virtual_key_position,
        .timestamp = timestamp,
    };

    sequence->is_pressed = true;
    return behavior_keymap_binding_pressed(&sequence->behavior, event);
}

static inline int release_leader_behavior(struct leader_seq_cfg *sequence, int32_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = sequence->virtual_key_position,
        .timestamp = timestamp,
    };

    sequence->is_pressed = false;
    return behavior_keymap_binding_released(&sequence->behavior, event);
}

static int stop_timer() {
    int timer_cancel_result = k_work_cancel_delayable(&release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        timer_cancelled = true;
    }
    return timer_cancel_result;
}

static void reset_timer(int32_t timestamp) {
    release_at = timestamp + timeout_ms;
    int32_t ms_left = release_at - k_uptime_get();
    if (ms_left > 0) {
        k_work_schedule(&release_timer, K_MSEC(ms_left));
        LOG_DBG("Successfully reset leader timer");
    }
}

void zmk_leader_activate(int32_t timeout, bool _timerless, uint32_t position) {
    LOG_DBG("leader key activated");
    leader_status = true;
    press_count = 0;
    release_count = 0;
    timeout_ms = timeout;
    active_leader_position = position;
    layer = zmk_keymap_highest_layer_active();
    first_release = false;
    timerless = _timerless;
    if (!timerless) {
        reset_timer(k_uptime_get());
    }
    for (int i = 0; i < CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE; i++) {
        leader_pressed_keys[i] = NULL;
    }
};

void zmk_leader_deactivate() {
    LOG_DBG("leader key deactivated");
    leader_status = false;
    clear_candidates();
};

void behavior_leader_key_timer_handler(struct k_work *item) {
    if (!leader_status) {
        return;
    }
    if (timer_cancelled) {
        return;
    }
    LOG_DBG("Leader deactivated due to timeout");
    for (int i = 0; i < num_comp_candidates; i++) {
        if (!completed_sequence_candidates[i]->is_pressed) {
            press_leader_behavior(completed_sequence_candidates[i], k_uptime_get());
            release_leader_behavior(completed_sequence_candidates[i], k_uptime_get());
        }
    }
    zmk_leader_deactivate();
}

static int position_state_changed_listener(const zmk_event_t *ev) {
    struct zmk_position_state_changed *data = as_zmk_position_state_changed(ev);
    if (data == NULL) {
        return 0;
    }

    if (!leader_status && !data->state && !all_keys_released()) {
        if (release_key_in_sequence(data->position)) {
            return ZMK_EV_EVENT_HANDLED;
        }
        return 0;
    }

    if (leader_status) {
        if (data->state) { // keydown
            leader_find_candidates(data->position, press_count);
            LOG_DBG("leader cands: %d comp: %d", num_candidates, num_comp_candidates);
            stop_timer();
            current_sequence[press_count] = data->position;
            leader_pressed_keys[press_count] = data;
            press_count++;
            for (int i = 0; i < num_comp_candidates; i++) {
                struct leader_seq_cfg *seq = completed_sequence_candidates[i];
                if (seq->immediate_trigger || (num_candidates == 1 && num_comp_candidates == 1)) {
                    press_leader_behavior(seq, data->timestamp);
                }
            }
        } else { // keyup
            if (data->position == active_leader_position && !first_release) {
                first_release = true;
                return 0;
            }
            if (!is_in_current_sequence(data->position)) {
                return 0;
            }
            if (num_candidates == 0) {
                zmk_leader_deactivate();
                return ZMK_EV_EVENT_HANDLED;
            }

            release_count++;
            release_key_in_sequence(data->position);

            for (int i = 0; i < num_comp_candidates; i++) {
                struct leader_seq_cfg *seq = completed_sequence_candidates[i];
                if (seq->is_pressed && all_keys_released()) {
                    release_leader_behavior(seq, data->timestamp);
                    num_comp_candidates--;
                }
                if (num_candidates == 1 && num_comp_candidates == 0) {
                    zmk_leader_deactivate();
                }
            }
            if (!timerless || num_comp_candidates < num_candidates) {
                reset_timer(data->timestamp);
            }
        }
        return ZMK_EV_EVENT_HANDLED;
    }

    return 0;
}

ZMK_LISTENER(leader, position_state_changed_listener);
ZMK_SUBSCRIPTION(leader, zmk_position_state_changed);

#define LEADER_INST(n)                                                                             \
    static struct leader_seq_cfg sequence_config_##n = {                                           \
        .virtual_key_position = ZMK_KEYMAP_LEN + __COUNTER__,                                      \
        .immediate_trigger = DT_PROP(n, immediate_trigger),                                        \
        .is_pressed = false,                                                                       \
        .key_positions = DT_PROP(n, key_positions),                                                \
        .key_position_len = DT_PROP_LEN(n, key_positions),                                         \
        .behavior = ZMK_KEYMAP_EXTRACT_BINDING(0, n),                                              \
        .layers = DT_PROP(n, layers),                                                              \
        .layers_len = DT_PROP_LEN(n, layers),                                                      \
    };

DT_INST_FOREACH_CHILD(0, LEADER_INST)

#define INTITIALIAZE_LEADER_SEQUENCES(n) intitialiaze_leader_sequences(&sequence_config_##n);

static int leader_init() {
    k_work_init_delayable(&release_timer, behavior_leader_key_timer_handler);
    DT_INST_FOREACH_CHILD(0, INTITIALIAZE_LEADER_SEQUENCES);
    return 0;
}

SYS_INIT(leader_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
