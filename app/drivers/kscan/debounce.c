/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "debounce.h"

static uint32_t get_threshold(const struct debounce_state *state,
                              const struct debounce_config *config) {
    return state->pressed ? config->debounce_release_ms : config->debounce_press_ms;
}

static void increment_counter(struct debounce_state *state, const int elapsed_ms) {
    if (state->counter + elapsed_ms > DEBOUNCE_COUNTER_MAX) {
        state->counter = DEBOUNCE_COUNTER_MAX;
    } else {
        state->counter += elapsed_ms;
    }
}

static void decrement_counter(struct debounce_state *state, const int elapsed_ms) {
    if (state->counter < elapsed_ms) {
        state->counter = 0;
    } else {
        state->counter -= elapsed_ms;
    }
}

void debounce_update(struct debounce_state *state, const bool active, const int elapsed_ms,
                     const struct debounce_config *config) {
    // This uses a variation of the integrator debouncing described at
    // https://www.kennethkuhn.com/electronics/debounce.c
    // Every update where "active" does not match the current state, we increment
    // a counter, otherwise we decrement it. When the counter reaches a
    // threshold, the state flips and we reset the counter.
    state->changed = false;

    if (active == state->pressed) {
        decrement_counter(state, elapsed_ms);
        return;
    }

    const uint32_t flip_threshold = get_threshold(state, config);

    if (state->counter < flip_threshold) {
        increment_counter(state, elapsed_ms);
        return;
    }

    state->pressed = !state->pressed;
    state->counter = 0;
    state->changed = true;
}

bool debounce_is_active(const struct debounce_state *state) {
    return state->pressed || state->counter > 0;
}

bool debounce_is_pressed(const struct debounce_state *state) { return state->pressed; }

bool debounce_get_changed(const struct debounce_state *state) { return state->changed; }