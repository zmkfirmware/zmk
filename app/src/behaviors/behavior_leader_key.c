/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_leader_key

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/virtual_key_position.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static bool leader_status;
static int32_t press_count;
static int32_t release_count;
static int32_t timeout_ms;
static int32_t active_leader_position;
static bool first_release;
static struct k_work_delayable release_timer;
static int64_t release_at;
// static bool timer_started;
static bool timer_cancelled;

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
static uint8_t source;
#endif

struct leader_seq_cfg {
    int32_t key_positions[CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE];
    int32_t key_position_len;

    bool immediate_trigger;
    bool is_pressed;

    // the virtual key position is a key position outside the range used by the keyboard.
    // it is necessary so hold-taps can uniquely identify a behavior.
    int32_t virtual_key_position;

    struct zmk_behavior_binding behavior;
};

struct behavior_leader_key_config {
    int32_t timeout_ms;
    struct leader_seq_cfg *sequences;
    size_t sequences_len;
};

// leader_pressed_keys is filled with an event when a key is pressed.
// The keys are removed from this array when they are released.
// Once this array is empty, the behavior is released.
static const struct zmk_position_state_changed
    *leader_pressed_keys[CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE] = {NULL};

static uint32_t current_sequence[CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE] = {-1};

// the set of candidate leader based on the currently leader_pressed_keys
static int num_candidates;

static struct leader_seq_cfg *sequence_candidates[CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY];
static int num_comp_candidates;
static struct leader_seq_cfg
    *completed_sequence_candidates[CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY];

const static struct behavior_leader_key_config *active_leader_cfg;

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
        if (leader_pressed_keys[i] != NULL) {
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

    for (int i = 0; i < active_leader_cfg->sequences_len; i++) {
        struct leader_seq_cfg *sequence = &(active_leader_cfg->sequences[i]);

        if (sequence == NULL) {
            continue;
        }

        if (sequence->key_positions[count] == position && has_current_sequence(sequence, count) &&
            !is_duplicate(sequence)) {
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
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = source,
#endif
        .timestamp = timestamp,
    };

    sequence->is_pressed = true;
    return behavior_keymap_binding_pressed(&sequence->behavior, event);
}

static inline int release_leader_behavior(struct leader_seq_cfg *sequence, int32_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = sequence->virtual_key_position,
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = source,
#endif
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

static void activate_leader_key(const struct behavior_leader_key_config *cfg, uint32_t position) {
    LOG_DBG("leader key activated");
    leader_status = true;
    press_count = 0;
    release_count = 0;
    timeout_ms = cfg->timeout_ms;
    active_leader_position = position;
    first_release = false;
    active_leader_cfg = cfg;

    if (timeout_ms > 0) {
        reset_timer(k_uptime_get());
    }

    for (int i = 0; i < CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE; i++) {
        leader_pressed_keys[i] = NULL;
    }
};

static void zmk_leader_deactivate() {
    LOG_DBG("leader key deactivated");
    leader_status = false;
    clear_candidates();
};

static void behavior_leader_key_timer_handler(struct k_work *item) {
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
                LOG_DBG("leader i is %d", i);
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

            if (timeout_ms > 0 || num_comp_candidates < num_candidates) {
                reset_timer(data->timestamp);
            }
        }
        return ZMK_EV_EVENT_HANDLED;
    }

    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_leader_key_config *cfg = dev->config;

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    source = event.source;
#endif

    activate_leader_key(cfg, event.position);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_leader_key_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

static int behavior_leader_key_init(const struct device *dev) {
    k_work_init_delayable(&release_timer, behavior_leader_key_timer_handler);
    return 0;
}

ZMK_LISTENER(leader, position_state_changed_listener);
ZMK_SUBSCRIPTION(leader, zmk_position_state_changed);

#define SEQUENCE_ITEM(i, n, prop) DT_PROP_BY_IDX(n, prop, i)

#define PROP_SEQUENCES(n, prop)                                                                    \
    {                                                                                              \
        .virtual_key_position = ZMK_VIRTUAL_KEY_POSITION_LEADER(__COUNTER__),                      \
        .is_pressed = false,                                                                       \
        .key_position_len = DT_PROP_LEN(n, prop),                                                  \
        .key_positions = {LISTIFY(DT_PROP_LEN(n, prop), SEQUENCE_ITEM, (, ), n, prop)},            \
        .behavior = ZMK_KEYMAP_EXTRACT_BINDING(0, n),                                              \
    }

#define LEAD_INST(n)                                                                               \
    static struct leader_seq_cfg leader_sequences_##n[] = {                                        \
        DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(n, PROP_SEQUENCES, (, ), key_positions)};      \
    static struct behavior_leader_key_config behavior_leader_key_config_##n = {                    \
        .sequences = leader_sequences_##n, .sequences_len = ARRAY_SIZE(leader_sequences_##n)};     \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_leader_key_init, NULL, NULL,                               \
                            &behavior_leader_key_config_##n, POST_KERNEL,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_leader_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LEAD_INST)
