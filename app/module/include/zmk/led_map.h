/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

/* Per-key RGB effect types */
enum per_key_effect {
    PER_KEY_EFFECT_OFF,
    PER_KEY_EFFECT_SOLID,
    PER_KEY_EFFECT_REACTIVE,
    PER_KEY_EFFECT_REACTIVE_FADE,
    PER_KEY_EFFECT_BREATHE,
    PER_KEY_EFFECT_SPECTRUM,
    PER_KEY_EFFECT_SWIRL,
    PER_KEY_EFFECT_NUMBER
};

/**
 * Turn on per-key RGB effects.
 * @return 0 on success, negative error code otherwise
 */
int zmk_led_map_on(void);

/**
 * Turn off per-key RGB effects.
 * @return 0 on success, negative error code otherwise
 */
int zmk_led_map_off(void);

/**
 * Toggle per-key RGB effects on/off.
 * @return 0 on success, negative error code otherwise
 */
int zmk_led_map_toggle(void);

/**
 * Select a per-key RGB effect.
 * @param effect The effect to select
 * @return 0 on success, negative error code otherwise
 */
int zmk_led_map_select_effect(enum per_key_effect effect);

/**
 * Cycle through per-key RGB effects.
 * @param direction +1 for next, -1 for previous
 * @return 0 on success, negative error code otherwise
 */
int zmk_led_map_cycle_effect(int direction);
