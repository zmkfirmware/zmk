#pragma once

int zmk_events_position_pressed(u32_t row, u32_t column);
int zmk_events_position_released(u32_t row, u32_t column);
int zmk_events_keycode_pressed(u32_t keycode);
int zmk_events_keycode_released(u32_t keycode);
int zmk_events_mod_pressed(u32_t modifier);
int zmk_events_mod_released(u32_t modifier);
int zmk_events_consumer_key_pressed(u32_t usage);
int zmk_events_consumer_key_released(u32_t usage);

// TODO: Encoders?
// TODO: Sensors?