#pragma once

#include <zmk/keys.h>

int zmk_events_position_pressed(u32_t position);
int zmk_events_position_released(u32_t position);
int zmk_events_keycode_pressed(u8_t usage_page, u32_t keycode);
int zmk_events_keycode_released(u8_t usage_page, u32_t keycode);
int zmk_events_modifiers_pressed(zmk_mod_flags modifiers);
int zmk_events_modifiers_released(zmk_mod_flags modifiers);
int zmk_events_consumer_key_pressed(u32_t usage);
int zmk_events_consumer_key_released(u32_t usage);

// TODO: Encoders?
// TODO: Sensors?